#include "dsp/digital.hpp"
#include "qwelk.hpp"


#define CHANNELS 4


struct ModuleColumn : Module {
    enum ParamIds {
        PARAM_MASTER,
        PARAM_AVG,
        PARAM_WEIGHTED,
        NUM_PARAMS
    };
    enum InputIds {
        IN_SIG,
        IN_UPSTREAM = IN_SIG + CHANNELS,
        NUM_INPUTS = IN_UPSTREAM + CHANNELS
    };
    enum OutputIds {
        OUT_SIDE,
        OUT_DOWNSTREAM = OUT_SIDE + CHANNELS,
        NUM_OUTPUTS = OUT_DOWNSTREAM + CHANNELS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    ModuleColumn() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

void ModuleColumn::step() {
    bool avg = params[PARAM_AVG].value == 0.0;
    bool weighted = params[PARAM_WEIGHTED].value == 0.0;

    float onno = 0;
    float dsum = 0.0;
    for (int i = 0; i < CHANNELS; ++i) {
        float usv = inputs[IN_UPSTREAM + i].value;
        float val = inputs[IN_SIG + i].value;
        
        outputs[OUT_SIDE + i].value = val;

        usv = avg ? (usv < 0.0 ? 0.0 : usv) : (usv);
        
        if (inputs[IN_UPSTREAM + i].active)
        {
            if (weighted)
                onno += usv;
            else if (usv != 0.0)
                onno += 1;
        }
        
        if (!weighted && inputs[IN_SIG + i].active && val != 0.0)
            onno += 1;

        float sum = weighted ? (usv * val) : (usv + val);

        dsum += sum;
        
        outputs[OUT_DOWNSTREAM + i].value = (avg && onno != 0) ? (dsum / onno) : (dsum);
    }
}


WidgetColumn::WidgetColumn() {
    ModuleColumn *module = new ModuleColumn();
    setModule(module);

    box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Column.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));

    addParam(createParam<CKSS>(Vec(3.5, 30), module, ModuleColumn::PARAM_AVG, 0.0, 1.0, 1.0));
    addParam(createParam<CKSS>(Vec(42, 30), module, ModuleColumn::PARAM_WEIGHTED, 0.0, 1.0, 1.0));

    float x = 2.5, xstep = 15, ystep = 23.5;
    for (int i = 0; i < CHANNELS; ++i)
    {
        float y = 80 + i * 80;
        addInput(createInput<PJ301MPort>(Vec(x + xstep, y - ystep), module, ModuleColumn::IN_UPSTREAM + i));
        addOutput(createOutput<PJ301MPort>(Vec(x + xstep*2, y), module, ModuleColumn::OUT_SIDE + i));
        addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleColumn::IN_SIG + i));
        addOutput(createOutput<PJ301MPort>(Vec(x + xstep, y + ystep), module, ModuleColumn::OUT_DOWNSTREAM + i));
    }
}

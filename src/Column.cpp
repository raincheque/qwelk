#include "dsp/digital.hpp"
#include "qwelk.hpp"


#define CHANNELS 4


struct ModuleColumn : Module {
    enum ParamIds {
        PARAM_MASTER,
        PARAM_AVG,
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

    int onno = 0;
    float dsum = 0.0;
    for (int i = 0; i < CHANNELS; ++i) {
        float usv = inputs[IN_UPSTREAM + i].value;
        float val = inputs[IN_SIG + i].value;

        if (inputs[IN_UPSTREAM + i].active && usv != 0.0)
            onno++;
        if (inputs[IN_SIG + i].active && val != 0.0)
            onno++;

        outputs[OUT_SIDE + i].value = val;

        float sum = usv + val;
        dsum += sum;
        
        if (avg && onno > 0)
            dsum = dsum / onno;
        
        outputs[OUT_DOWNSTREAM + i].value = dsum;
    }
}


WidgetColumn::WidgetColumn() {
    ModuleColumn *module = new ModuleColumn();
    setModule(module);

    box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank_8.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));

    addParam(createParam<CKSS>(Vec(3, 30), module, ModuleColumn::PARAM_AVG, 0.0, 1.0, 1.0));

    float x = 7.5, xstep = 25, ystep = 20;
    for (int i = 0; i < CHANNELS; ++i)
    {
        float y = 90 + i * 70;
        addInput(createInput<PJ301MPort>(Vec(x + xstep, y - ystep), module, ModuleColumn::IN_UPSTREAM + i));
        addOutput(createOutput<PJ301MPort>(Vec(x + xstep*2, y), module, ModuleColumn::OUT_SIDE + i));
        addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleColumn::IN_SIG + i));
        addOutput(createOutput<PJ301MPort>(Vec(x + xstep, y + ystep), module, ModuleColumn::OUT_DOWNSTREAM + i));
    }
}

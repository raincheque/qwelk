#include "dsp/decimator.hpp"
#include "dsp/filter.hpp"
#include "math.hpp"
#include "util.hpp"
#include "qwelk.hpp"


#define COMPONENTS 4


struct ModuleIndra : Module {
    enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
        IN_PITCH,
        IN_PHASE,
		NUM_INPUTS = IN_PHASE + COMPONENTS
	};
	enum OutputIds {
        OUT_COMPONENT,
		NUM_OUTPUTS = OUT_COMPONENT + COMPONENTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    float offset[COMPONENTS] {};
    float phase[COMPONENTS] {};

    ModuleIndra() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};


void ModuleIndra::step()
{
    auto p = inputs[IN_PITCH].value;
    auto f = 261.626 * powf(2.0, p / 12.0);
    for (int i = 0; i < COMPONENTS; ++i) {
        if (inputs[IN_PHASE + i].active) {
            auto no = clampf(inputs[IN_PHASE + i].value, 0, 10.0) / 10.0;
            offset[i] = no;
        }

        phase[i] += (f * (i + 1)) * (1.0 / engineGetSampleRate());
        while (phase[i] > 1.0)
            phase[i] -= 1.0;

        auto o = offset[i];
        auto v = sinf(2 * M_PI * (phase[i] + o));
        
        outputs[OUT_COMPONENT + i].value = 5.0 * v;
    }
}


WidgetIndra::WidgetIndra() {
    ModuleIndra *module = new ModuleIndra();
    setModule(module);

    box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank_8.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

    float x = box.size.x / 4 - 30, y = 30;
    addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleIndra::IN_PITCH));
    for (int i = 0; i < COMPONENTS; ++i) {
        addInput(createInput<PJ301MPort>(Vec(x + 30 * i, y + 30), module, ModuleIndra::IN_PHASE + i));
        addOutput(createOutput<PJ301MPort>(Vec(x + 30 * i, y + 60), module, ModuleIndra::OUT_COMPONENT + i));
    }
}

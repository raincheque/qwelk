#include "qwelk.hpp"


#define CHANNELS 3


struct ModuleGate : Module {
    enum ParamIds {
        PARAM_THRESHOLD,
        PARAM_OUTGAIN = PARAM_THRESHOLD + CHANNELS,
        NUM_PARAMS = PARAM_OUTGAIN + CHANNELS
    };
    enum InputIds {
        IN_SIG,
        NUM_INPUTS = IN_SIG + CHANNELS
    };
    enum OutputIds {
        OUT_GATE,
        NUM_OUTPUTS = OUT_GATE + CHANNELS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    ModuleGate() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

void ModuleGate::step() {
    for (int i = 0; i < CHANNELS; ++i) {
        float in = inputs[IN_SIG + i].value;
        float threshold = params[PARAM_THRESHOLD + i].value;
        float out_gain = params[PARAM_OUTGAIN + i].value;
        float out = in > threshold ? 10.0 : 0.0;
        outputs[OUT_GATE + i].value = out * out_gain;
    }
}


struct RoundTinyKnob : RoundBlackKnob {
	RoundTinyKnob()
    {
		box.size = Vec(20, 20);
	}
};

WidgetGate::WidgetGate() {
    ModuleGate *module = new ModuleGate();
    setModule(module);

    box.size = Vec(2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank_8.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));

    for (int i = 0; i < CHANNELS; ++i) {
        float x = 2.5, top = i * 120;
        addParam(createParam<RoundTinyKnob>(Vec(x + 2   , top +  10), module, ModuleGate::PARAM_THRESHOLD + i, -10.0, 10.0, 0));
        addInput(createInput<PJ301MPort>(   Vec(x       , top +  35), module, ModuleGate::IN_SIG + i));
        addParam(createParam<RoundTinyKnob>(Vec(x + 2   , top +  65), module, ModuleGate::PARAM_OUTGAIN + i, 0, 1.0, 1.0));
        addOutput(createOutput<PJ301MPort>( Vec(x       , top +  90), module, ModuleGate::OUT_GATE + i));
    }
}


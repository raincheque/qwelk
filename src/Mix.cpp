#include "dsp/digital.hpp"
#include "qwelk.hpp"

struct ModuleMix : Module {
    enum ParamIds {
        PARAM_GAIN_M,
        PARAM_GAIN_S,
        PARAM_GAIN_MS,
        NUM_PARAMS
    };
    enum InputIds {
        IN_L,
        IN_R,
        IN_M,
        IN_S,
        IN_GAIN_M,
        IN_GAIN_S,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_M,
        OUT_S,
        OUT_L,
        OUT_R,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    ModuleMix() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

void ModuleMix::step() {
    if (inputs[IN_L].active && inputs[IN_R].active) {
        float iam = inputs[IN_GAIN_M].value / 10.0;
        float ias = inputs[IN_GAIN_S].value / 10.0;
        float ams = params[PARAM_GAIN_MS].value;
        float am = inputs[IN_GAIN_M].active ? params[PARAM_GAIN_M].value * iam : params[PARAM_GAIN_M].value;
        float as = inputs[IN_GAIN_S].active ? params[PARAM_GAIN_S].value * ias : params[PARAM_GAIN_S].value;
        float l = inputs[IN_L].value;
        float r = inputs[IN_R].value;
        float m = l + r;
        float s = l - r;
        outputs[OUT_M].value = m * ams * am;
        outputs[OUT_S].value = s * ams * as;
    }
    if (inputs[IN_M].active && inputs[IN_S].active) {
        float m = inputs[IN_M].value;
        float s = inputs[IN_S].value;
        float l = (m + s) * 0.5;
        float r = (m - s) * 0.5;
        outputs[OUT_L].value = l;
        outputs[OUT_R].value = r;
    }
}

struct RoundTinyKnob : RoundBlackKnob {
	RoundTinyKnob()
    {
		box.size = Vec(20, 20);
	}
};

WidgetMix::WidgetMix() {
    ModuleMix *module = new ModuleMix();
    setModule(module);

    box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank_8.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));

    float x = box.size.x / 2.0 - 30;
    
    addInput(createInput<PJ301MPort>(Vec(x  ,   30), module, ModuleMix::IN_L));
    addInput(createInput<PJ301MPort>(Vec(x  ,   60), module, ModuleMix::IN_R));
    addParam(createParam<RoundTinyKnob>(Vec(x,  90), module, ModuleMix::PARAM_GAIN_MS, 0.0, 1.0, 1.0));
    addOutput(createOutput<PJ301MPort>(Vec(x,  120), module, ModuleMix::OUT_M));
    addParam(createParam<RoundTinyKnob>(Vec(x, 150), module, ModuleMix::PARAM_GAIN_M, 0.0, 1.0, 1.0));
    addInput(createInput<PJ301MPort>(Vec(x + 25 ,  150), module, ModuleMix::IN_GAIN_M));
    addOutput(createOutput<PJ301MPort>(Vec(x,  180), module, ModuleMix::OUT_S));
    addParam(createParam<RoundTinyKnob>(Vec(x, 210), module, ModuleMix::PARAM_GAIN_S, 0.0, 1.0, 1.0));
    addInput(createInput<PJ301MPort>(Vec(x + 25 ,  210), module, ModuleMix::IN_GAIN_S));

    addInput(createInput<PJ301MPort>(Vec(x  , 240), module, ModuleMix::IN_M));
    addInput(createInput<PJ301MPort>(Vec(x  , 270), module, ModuleMix::IN_S));
    addOutput(createOutput<PJ301MPort>(Vec(x, 300), module, ModuleMix::OUT_L));
    addOutput(createOutput<PJ301MPort>(Vec(x, 330), module, ModuleMix::OUT_R));
}

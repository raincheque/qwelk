#include "qwelk.hpp"
#include "qwelk_common.h"


struct ModuleMix : Module {
    enum ParamIds {
        PARAM_GAIN_M,
        PARAM_GAIN_S,
        PARAM_GAIN_MS,
        PARAM_GAIN_L,
        PARAM_GAIN_R,
        PARAM_GAIN_LR,
        NUM_PARAMS
    };
    enum InputIds {
        IN_L,
        IN_R,
        IN_M,
        IN_S,
        IN_GAIN_M,
        IN_GAIN_S,
        IN_GAIN_L,
        IN_GAIN_R,
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

  ModuleMix() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configParam(ModuleMix::PARAM_GAIN_MS, 0.0, 1.0, 1.0, "");
    configParam(ModuleMix::PARAM_GAIN_M, 0.0, 1.0, 1.0, "");
    configParam(ModuleMix::PARAM_GAIN_S, 0.0, 1.0, 1.0, "");
    configParam(ModuleMix::PARAM_GAIN_LR, 0.0, 1.0, 1.0, "");
    configParam(ModuleMix::PARAM_GAIN_L, 0.0, 1.0, 1.0, "");
    configParam(ModuleMix::PARAM_GAIN_R, 0.0, 1.0, 1.0, "");
  }
    void process(const ProcessArgs& args) override;
};
static inline float _max(float a, float b) {return a < b ? b : a;}
void ModuleMix::process(const ProcessArgs& args) {
    if (inputs[IN_L].isConnected() && inputs[IN_R].isConnected()) {
        float iam = _max(inputs[IN_GAIN_M].getVoltage(), 0.f) / 10.0;
        float ias = _max(inputs[IN_GAIN_S].getVoltage(), 0.f) / 10.0;
        float ams = params[PARAM_GAIN_MS].getValue();
        float am = inputs[IN_GAIN_M].isConnected() ? params[PARAM_GAIN_M].getValue() * iam : params[PARAM_GAIN_M].getValue();
        float as = inputs[IN_GAIN_S].isConnected() ? params[PARAM_GAIN_S].getValue() * ias : params[PARAM_GAIN_S].getValue();
        float l = inputs[IN_L].getVoltage();
        float r = inputs[IN_R].getVoltage();
        float m = l + r;
        float s = l - r;
        outputs[OUT_M].setVoltage(m * ams * am);
        outputs[OUT_S].setVoltage(s * ams * as);
    }
    if (inputs[IN_M].isConnected() && inputs[IN_S].isConnected()) {
        float ial = _max(inputs[IN_GAIN_L].getVoltage(), 0.f) / 10.0;
        float iar = _max(inputs[IN_GAIN_R].getVoltage(), 0.f) / 10.0;
        float alr = params[PARAM_GAIN_LR].getValue();
        float al = inputs[IN_GAIN_L].isConnected() ? params[PARAM_GAIN_L].getValue() * ial : params[PARAM_GAIN_L].getValue();
        float ar = inputs[IN_GAIN_R].isConnected() ? params[PARAM_GAIN_R].getValue() * iar : params[PARAM_GAIN_R].getValue();
        float m = inputs[IN_M].getVoltage();
        float s = inputs[IN_S].getVoltage();
        float l = (m + s) * 0.5;
        float r = (m - s) * 0.5;
        outputs[OUT_L].setVoltage(l * alr * al);
        outputs[OUT_R].setVoltage(r * alr * ar);
    }
}

struct WidgetMix : ModuleWidget {
  WidgetMix(ModuleMix *module) {
		setModule(module);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mix.svg")));

    addChild(createWidget<ScrewSilver>(Vec(5, 0)));
    addChild(createWidget<ScrewSilver>(Vec(5, 365)));

    float x = box.size.x / 2.0 - 27;

    addInput(createInput<PJ301MPort>(Vec(x     ,   25), module, ModuleMix::IN_L));
    addInput(createInput<PJ301MPort>(Vec(x + 30,   25), module, ModuleMix::IN_R));

    addParam(createParam<SmallKnob>(Vec(x + 28,  55), module, ModuleMix::PARAM_GAIN_MS));

    addParam(createParam<TinyKnob>(Vec(x    , 90), module, ModuleMix::PARAM_GAIN_M));
    addInput(createInput<PJ301MPort>(Vec(x + 30  , 88), module, ModuleMix::IN_GAIN_M));
    addOutput(createOutput<PJ301MPort>(Vec(x + 30, 113), module, ModuleMix::OUT_M));

    addParam(createParam<TinyKnob>(Vec(x    , 147), module, ModuleMix::PARAM_GAIN_S));
    addInput(createInput<PJ301MPort>(Vec(x + 30  , 145), module, ModuleMix::IN_GAIN_S));
    addOutput(createOutput<PJ301MPort>(Vec(x + 30, 169), module, ModuleMix::OUT_S));

    addInput(createInput<PJ301MPort>(Vec(x     , 210), module, ModuleMix::IN_M));
    addInput(createInput<PJ301MPort>(Vec(x + 30, 210), module, ModuleMix::IN_S));

    addParam(createParam<SmallKnob>(Vec(x + 28,  240), module, ModuleMix::PARAM_GAIN_LR));

    addParam(createParam<TinyKnob>(Vec(x    , 275), module, ModuleMix::PARAM_GAIN_L));
    addInput(createInput<PJ301MPort>(Vec(x + 30  , 273), module, ModuleMix::IN_GAIN_L));
    addOutput(createOutput<PJ301MPort>(Vec(x + 30, 298), module, ModuleMix::OUT_L));

    addParam(createParam<TinyKnob>(Vec(x    , 332), module, ModuleMix::PARAM_GAIN_R));
    addInput(createInput<PJ301MPort>(Vec(x + 30  , 330), module, ModuleMix::IN_GAIN_R));
    addOutput(createOutput<PJ301MPort>(Vec(x + 30, 355), module, ModuleMix::OUT_R));
  }
};

Model *modelMix = createModel<ModuleMix, WidgetMix>("Mix");

#include "qwelk.hpp"
#include "qwelk_common.h"

#define CHANNELS 2


struct ModuleGate : Module {
    enum ParamIds {
        PARAM_GATEMODE,
        PARAM_THRESHOLD = PARAM_GATEMODE + CHANNELS,
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

    ModuleGate() {
		  config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
      for (int i = 0; i < CHANNELS; ++i) {
        configParam(ModuleGate::PARAM_GATEMODE + i, 0.0, 1.0, 1.0, "");
        configParam(ModuleGate::PARAM_THRESHOLD + i, -10.0, 10.0, 0, "");
        configParam(ModuleGate::PARAM_OUTGAIN + i, -1.0, 1.0, 1.0, "");
      }
    }
    void process(const ProcessArgs& args) override;
};

void ModuleGate::process(const ProcessArgs& args) {
    for (int i = 0; i < CHANNELS; ++i) {
        bool gatemode = params[PARAM_GATEMODE + i].getValue() > 0.0;
        float in = inputs[IN_SIG + i].getVoltage();
        float threshold = params[PARAM_THRESHOLD + i].getValue();
        float out_gain = params[PARAM_OUTGAIN + i].getValue();
        float out = ((threshold >= 0) ? (in > threshold) : (in < threshold))
                    ? (gatemode ? 10.0 : in) : 0.0;
        outputs[OUT_GATE + i].setVoltage(out * out_gain);
    }
}

struct WidgetGate : ModuleWidget {
  WidgetGate(ModuleGate *module) {

		setModule(module);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Gate.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));

    for (int i = 0; i < CHANNELS; ++i) {
        float x = 2.5, top = 45 + i * 158;
        addParam(createParam<CKSS>(         Vec(x + 5.7, top +   8), module, ModuleGate::PARAM_GATEMODE + i));
        addParam(createParam<TinyKnob>(Vec(x + 2.5, top +  40), module, ModuleGate::PARAM_THRESHOLD + i));
        addInput(createInput<PJ301MPort>(   Vec(x      , top +  63), module, ModuleGate::IN_SIG + i));
        addParam(createParam<TinyKnob>(Vec(x + 2.5, top + 102), module, ModuleGate::PARAM_OUTGAIN + i));
        addOutput(createOutput<PJ301MPort>( Vec(x      , top + 125), module, ModuleGate::OUT_GATE + i));
    }
  }
};

Model *modelGate = createModel<ModuleGate, WidgetGate>("Gate");

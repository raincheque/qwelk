#include "qwelk.hpp"

#define CHANNELS 8


struct ModuleNot : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        INPUT_SIG,
        NUM_INPUTS = INPUT_SIG + CHANNELS
    };
    enum OutputIds {
        OUTPUT_NOT,
        NUM_OUTPUTS = OUTPUT_NOT + CHANNELS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    ModuleNot() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);}
    void process(const ProcessArgs& args) override;
};

void ModuleNot::process(const ProcessArgs& args) {
    for (int i = 0; i < CHANNELS; ++i) {
        outputs[OUTPUT_NOT + i].setVoltage(inputs[INPUT_SIG + i].getVoltage() != 0.0 ? 0.0 : 10.0);
    }
}

struct WidgetNot : ModuleWidget {
  WidgetNot(ModuleNot *module) {

		setModule(module);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Not.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));

    float x = box.size.x / 2.0 - 25, ytop = 45, ystep = 39;
    for (int i = 0; i < CHANNELS; ++i) {
        addInput(createInput<PJ301MPort>(   Vec(x       , ytop + ystep * i), module, ModuleNot::INPUT_SIG  + i));
        addOutput(createOutput<PJ301MPort>( Vec(x + 26  , ytop + ystep * i), module, ModuleNot::OUTPUT_NOT + i));
    }
  }
};

Model *modelNot = createModel<ModuleNot, WidgetNot>("NOT");

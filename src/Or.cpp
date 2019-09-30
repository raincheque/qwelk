#include "qwelk.hpp"

#define CHANNELS 8


struct ModuleOr : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        INPUT_CHANNEL,
        NUM_INPUTS = INPUT_CHANNEL + CHANNELS
    };
    enum OutputIds {
        OUTPUT_OR,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    ModuleOr() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);}
    void process(const ProcessArgs& args) override;
};

void ModuleOr::process(const ProcessArgs& args) {
    int gate_on = 0;
    for (int i = 0; !gate_on && i < CHANNELS; ++i)
        gate_on = inputs[INPUT_CHANNEL + i].getVoltage();
    outputs[OUTPUT_OR].setVoltage(gate_on ? 10 : 0);
}

struct WidgetOr : ModuleWidget {
  WidgetOr(ModuleOr *module) {

		setModule(module);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Or.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));

    float x = box.size.x / 2.0 - 12, ytop = 45, ystep = 32.85;
    for (int i = 0; i < CHANNELS; ++i)
        addInput(createInput<PJ301MPort>(Vec(x, ytop + ystep * i), module, ModuleOr::INPUT_CHANNEL + i));
    ytop += 9;
    addOutput(createOutput<PJ301MPort>( Vec(x, ytop + ystep * CHANNELS), module, ModuleOr::OUTPUT_OR));
  }
};

Model *modelOr = createModel<ModuleOr, WidgetOr>("OR");

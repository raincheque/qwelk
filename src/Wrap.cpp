#include "qwelk.hpp"

#define CHANNELS 8


struct ModuleWrap : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        IN_WRAP,
        IN_SIG,
        NUM_INPUTS = IN_SIG + CHANNELS
    };
    enum OutputIds {
        OUT_WRAPPED,
        NUM_OUTPUTS = OUT_WRAPPED + CHANNELS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    int _wrap = -10;
    
    ModuleWrap() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);}
    void process(const ProcessArgs& args) override;
};

void ModuleWrap::process(const ProcessArgs& args) {
    int wrap = (clampSafe(inputs[IN_WRAP].getVoltage(), -5.0, 5.0) / 5.0) * (CHANNELS - 1);

    for (int i = 0; i < CHANNELS; ++i) {
        int w = i;
        if (wrap > 0)
            w = (i + wrap) % CHANNELS;
        else if (wrap < 0)
            w = (i + CHANNELS - wrap) % CHANNELS;
        outputs[OUT_WRAPPED + i].setVoltage(inputs[IN_SIG + w].getVoltage());
    }
}

struct WidgetWrap : ModuleWidget {
  WidgetWrap(ModuleWrap *module) {

		setModule(module);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Wrap.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));

    float x = box.size.x / 2.0 - 25, ytop = 60, ystep = 39;

    addInput(createInput<PJ301MPort>(Vec(17.5, 30), module, ModuleWrap::IN_WRAP));
    
    for (int i = 0; i < CHANNELS; ++i) {
        addInput(createInput<PJ301MPort>(  Vec(x       , ytop + ystep * i), module, ModuleWrap::IN_SIG  + i));
        addOutput(createOutput<PJ301MPort>( Vec(x + 26  , ytop + ystep * i), module, ModuleWrap::OUT_WRAPPED + i));
    }
  }
};

Model *modelWrap = createModel<ModuleWrap, WidgetWrap>("Wrap");

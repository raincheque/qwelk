#include "qwelk.hpp"

#define CHANNELS 2


struct ModuleXFade : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        INPUT_A,
        INPUT_B     = INPUT_A + CHANNELS,
        INPUT_X     = INPUT_B + CHANNELS,
        NUM_INPUTS  = INPUT_X + CHANNELS
    };
    enum OutputIds {
        OUTPUT_BLEND,
        NUM_OUTPUTS = OUTPUT_BLEND + CHANNELS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    ModuleXFade() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);}
    void process(const ProcessArgs& args) override;
};

void ModuleXFade::process(const ProcessArgs& args) {
    for (int i = 0; i < CHANNELS; ++i) {
        float blend = inputs[INPUT_X + i].getVoltage() / 10.0;
        outputs[OUTPUT_BLEND + i].setVoltage((1.0 - blend) * inputs[INPUT_A + i].getVoltage() + inputs[INPUT_B + i].getVoltage() * blend);
    }
}

struct WidgetXFade : ModuleWidget {
  WidgetXFade(ModuleXFade *module) {

		setModule(module);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/XFade.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));

    float x = box.size.x / 2.0 - 12, ytop = 45, ystep = 37.5;
    for (int i = 0; i < CHANNELS; ++i) {
        addInput(createInput<PJ301MPort>(   Vec(x, ytop + ystep * i), module, ModuleXFade::INPUT_A + i));
        addInput(createInput<PJ301MPort>(   Vec(x, ytop + ystep*1 + ystep * i), module, ModuleXFade::INPUT_B + i));
        addInput(createInput<PJ301MPort>(   Vec(x, ytop + ystep*2 + ystep * i), module, ModuleXFade::INPUT_X + i));
        addOutput(createOutput<PJ301MPort>( Vec(x, ytop + ystep*3 + ystep  * i), module, ModuleXFade::OUTPUT_BLEND + i));
        ytop += 4 * ystep - 17.5;
    }
  }
};

Model *modelXFade = createModel<ModuleXFade, WidgetXFade>("XFade");

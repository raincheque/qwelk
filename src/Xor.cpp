#include "qwelk.hpp"

#define CHANNELS 3


struct ModuleXor : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        INPUT_A,
        INPUT_B     = INPUT_A + CHANNELS,
        NUM_INPUTS  = INPUT_B + CHANNELS
    };
    enum OutputIds {
        OUTPUT_XOR,
        NUM_OUTPUTS = OUTPUT_XOR + CHANNELS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    ModuleXor() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);}
    void process(const ProcessArgs& args) override;
};

void ModuleXor::process(const ProcessArgs& args) {
    for (int i = 0; i < CHANNELS; ++i)
        outputs[OUTPUT_XOR + i].setVoltage(inputs[INPUT_A + i].getVoltage() == inputs[INPUT_B + i].getVoltage() ? 0.0 : 10.0);
}

struct WidgetXor : ModuleWidget {
  WidgetXor(ModuleXor *module) {
		setModule(module);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Xor.svg")));


    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));

    float x = box.size.x / 2.0 - 12, ytop = 45, ystep = 37.5;
    for (int i = 0; i < CHANNELS; ++i) {
        addInput(createInput<PJ301MPort>(   Vec(x, ytop + ystep * i), module, ModuleXor::INPUT_A + i));
        addInput(createInput<PJ301MPort>(   Vec(x, ytop + ystep*1 + ystep * i), module, ModuleXor::INPUT_B + i));
        addOutput(createOutput<PJ301MPort>( Vec(x, ytop + ystep*2 + ystep  * i), module, ModuleXor::OUTPUT_XOR + i));
        ytop += 3 * ystep - 42.5;
    }
  }
};

Model *modelXor = createModel<ModuleXor, WidgetXor>("XOR");

#include "qwelk.hpp"


struct ModuleScaler : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        INPUT_SUB_5,
        INPUT_MUL_2,
        INPUT_DIV_2,
        INPUT_ADD_5,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT_SUB_5,
        OUTPUT_MUL_2,
        OUTPUT_DIV_2,
        OUTPUT_ADD_5,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    ModuleScaler() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);}
    void process(const ProcessArgs& args) override;
};

void ModuleScaler::process(const ProcessArgs& args) {
    outputs[OUTPUT_SUB_5].setVoltage(inputs[INPUT_SUB_5].getVoltage() - 5.0);
    outputs[OUTPUT_MUL_2].setVoltage(inputs[INPUT_MUL_2].getNormalVoltage(outputs[OUTPUT_SUB_5].getVoltage()) * 2.0);
    outputs[OUTPUT_DIV_2].setVoltage(inputs[INPUT_DIV_2].getNormalVoltage(outputs[OUTPUT_MUL_2].getVoltage()) * 0.5);
    outputs[OUTPUT_ADD_5].setVoltage(inputs[INPUT_ADD_5].getNormalVoltage(outputs[OUTPUT_DIV_2].getVoltage()) + 5.0);
}

struct WidgetScaler : ModuleWidget {
  WidgetScaler(ModuleScaler *module) {

		setModule(module);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Scaler.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));

    float x = box.size.x / 2.0 - 12, y = 0, ytop = 30, ystep = 30, mstep = 16;
    addInput(createInput<PJ301MPort>(   Vec(x, ytop + (y+=ystep)), module, ModuleScaler::INPUT_SUB_5));
    addOutput(createOutput<PJ301MPort>( Vec(x, ytop + (y+=ystep)), module, ModuleScaler::OUTPUT_SUB_5));
    ytop += mstep;
    addInput(createInput<PJ301MPort>(   Vec(x, ytop + (y+=ystep)), module, ModuleScaler::INPUT_MUL_2));
    addOutput(createOutput<PJ301MPort>( Vec(x, ytop + (y+=ystep)), module, ModuleScaler::OUTPUT_MUL_2));
    ytop += mstep;
    addInput(createInput<PJ301MPort>(   Vec(x, ytop + (y+=ystep)), module, ModuleScaler::INPUT_DIV_2));
    addOutput(createOutput<PJ301MPort>( Vec(x, ytop + (y+=ystep)), module, ModuleScaler::OUTPUT_DIV_2));
    ytop += mstep;
    addInput(createInput<PJ301MPort>(   Vec(x, ytop + (y+=ystep)), module, ModuleScaler::INPUT_ADD_5));
    addOutput(createOutput<PJ301MPort>( Vec(x, ytop + (y+=ystep)), module, ModuleScaler::OUTPUT_ADD_5));
  }
};

Model *modelScaler = createModel<ModuleScaler, WidgetScaler>("Scaler");

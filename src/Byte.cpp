#include "qwelk.hpp"


#define CHANNELS 8


struct ModuleByte : Module {
    enum ParamIds {
        PARAM_SCAN,
        NUM_PARAMS
    };
    enum InputIds {
        INPUT_SCAN,
        INPUT_GATE,
        NUM_INPUTS = INPUT_GATE + CHANNELS
    };
    enum OutputIds {
        OUTPUT_COUNT,
        OUTPUT_NUMBER,
        NUM_OUTPUTS
    };
    enum LightIds {
        LIGHT_POS_SCAN,
        LIGHT_NEG_SCAN,
        NUM_LIGHTS,
    };

    int             scan = 1;
    int             scan_sign = 0;
    dsp::SchmittTrigger  trig_scan_input;
    dsp::SchmittTrigger  trig_scan_manual;

    const float output_volt_uni = 10.0;

    ModuleByte() {
		  config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
      configParam(ModuleByte::PARAM_SCAN, 0.0, 1.0, 0.0, "");
    }

    void process(const ProcessArgs& args) override;
};

void ModuleByte::process(const ProcessArgs& args)
{
    // determine scan direction
    int scan_input_sign = (int)sgn(inputs[INPUT_SCAN].getNormalVoltage(scan));
    if (scan_input_sign != scan_sign)
        scan = scan_sign = scan_input_sign;
    // manual tinkering with step?
    if (trig_scan_manual.process(params[PARAM_SCAN].getValue()))
        scan *= -1;

    int active_count = 0, count = 0, number = 0;
    for (int i = 0; i < CHANNELS; ++i) {
        int bit = scan >= 0 ? i : (CHANNELS - 1 - i);
        int above0 = inputs[INPUT_GATE + i].getVoltage() > 0 ? 1 : 0;
        active_count += inputs[INPUT_GATE + i].isConnected() ? 1 : 0;
        count += above0;
        if (above0)
            number |= 1 << bit;
    }

    outputs[OUTPUT_COUNT].setVoltage(active_count ? ((float)count  / active_count) * output_volt_uni : 0.0f);
    outputs[OUTPUT_NUMBER].setVoltage(((float)number / (float)(1 << (CHANNELS - 1))) * output_volt_uni);

    // indicate step direction
    lights[LIGHT_POS_SCAN].setBrightness(scan < 0 ? 0.0 : 0.9);
    lights[LIGHT_NEG_SCAN].setBrightness(scan < 0 ? 0.9 : 0.0);
}



template <typename _BASE>
struct MuteLight : _BASE {
    MuteLight()
    {
        this->box.size = mm2px(Vec(6, 6));
    }
};

struct WidgetByte : ModuleWidget {
  WidgetByte(ModuleByte *module) {

		setModule(module);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Byte.svg")));

    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

    const float ypad = 27.5;
    float x = box.size.x / 2.0 - 25.0 / 2.0;
    float ytop = 90.5;

    addParam(createParam<LEDBezel>(Vec(x + 1.5, ytop - ypad * 2 - 3.5), module, ModuleByte::PARAM_SCAN));
    addChild(createLight<MuteLight<GreenRedLight>>(Vec(x + 3.75, ytop - ypad * 2 + - 3.5 + 2), module, ModuleByte::LIGHT_POS_SCAN));
    addInput(createInput<PJ301MPort>(Vec(x, ytop - ypad + 1), module, ModuleByte::INPUT_SCAN));
    //ytop += ypad * 0.25;

    for (int i = 0; i < CHANNELS; ++i) {
        addInput(createInput<PJ301MPort>(Vec(x, ytop + ypad * i), module, ModuleByte::INPUT_GATE + i));
    }
    //ytop += ypad * 0.25;

    const float output_y = ytop + ypad * CHANNELS;

    addOutput(createOutput<PJ301MPort>(Vec(x, output_y        ), module, ModuleByte::OUTPUT_NUMBER));
    addOutput(createOutput<PJ301MPort>(Vec(x, output_y + ypad ), module, ModuleByte::OUTPUT_COUNT));
  }
};

Model *modelByte = createModel<ModuleByte, WidgetByte>("Byte");

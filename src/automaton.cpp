#include "dsp/digital.hpp"
#include "qwelk.hpp"

#define CHANNELS 8

struct ModuleAutomaton : Module {
    enum ParamIds {
        PARAM_LIGHT,
        NUM_PARAMS = PARAM_LIGHT + CHANNELS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT_ANY,
        OUTPUT_NUMON,
        OUTPUT_CELL,
        NUM_OUTPUTS = OUTPUT_CELL + CHANNELS
    };
    enum LightIds {
        LIGHT_MUTE,
        NUM_LIGHTS = LIGHT_MUTE + CHANNELS
    };

    int states[CHANNELS] {};
    SchmittTrigger trigs[CHANNELS];
    
    ModuleAutomaton() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

void ModuleAutomaton::step() {
    for (int i = 0; i < CHANNELS; ++i)
        if (trigs[i].process(params[PARAM_LIGHT + i].value))
            states[i] ^= 1;
    
    for (int i = 0; i < CHANNELS; ++i)
        lights[LIGHT_MUTE + i].setBrightness(states[i] ? 0.9 : 0.0);
    
    const float output_volt = 5.0;
    const float output_volt_uni = output_volt * 2;
    for (int i = 0; i < CHANNELS; ++i)
        outputs[OUTPUT_CELL + i].value = states[i] ? output_volt : 0.0;

    int oncount = 0;
    for (int i = 0; i < CHANNELS; ++i)
        oncount += states[i];
    outputs[OUTPUT_ANY].value = oncount ? output_volt : 0.0;
    outputs[OUTPUT_NUMON].value = ((float)oncount / (float)CHANNELS) * output_volt_uni;
}



template <typename _BASE>
struct MuteLight : _BASE {
    MuteLight() {
        this->box.size = mm2px(Vec(6, 6));
    }
};


WidgetAutomaton::WidgetAutomaton() {
    ModuleAutomaton *module = new ModuleAutomaton();
    setModule(module);

    box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank_8.svg")));
        addChild(panel);
    }

    // setPanel(SVG::load(assetPlugin(plugin, "res/Blank.svg")));

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));
    
    // addParam(createParam<Davies1900hBlackKnob>(Vec(28, 87), module, MyModule::PITCH_PARAM, -3.0, 3.0, 0.0));
    // addInput(createInput<PJ301MPort>(Vec(33, 186), module, MyModule::PITCH_INPUT));
    // addChild(createLight<MediumLight<RedLight>>(Vec(41, 59), module, MyModule::BLINK_LIGHT));
    
    const float ypad = 30;
    const float ytop = 50;
    const float lghx = box.size.x / 2.0 - 10.0;
    const float tlpx = 2.25;
    const float tlpy = 1.75;
    for (int i = 0; i < CHANNELS; ++i) {
        addParam(createParam<LEDBezel>(Vec(lghx, ytop + ypad * i), module, ModuleAutomaton::PARAM_LIGHT + i, 0.0, 1.0, 0.0));
        addChild(createLight<MuteLight<GreenLight>>(Vec(lghx + tlpx, ytop + tlpy + ypad * i), module, ModuleAutomaton::LIGHT_MUTE + i));

        addOutput(createOutput<PJ301MPort>(Vec(lghx + 25, ytop + ypad * i), module, ModuleAutomaton::OUTPUT_CELL + i));
    }
    addOutput(createOutput<PJ301MPort>(Vec(lghx, 320), module, ModuleAutomaton::OUTPUT_NUMON));
    addOutput(createOutput<PJ301MPort>(Vec(lghx + 25, 320), module, ModuleAutomaton::OUTPUT_ANY));
}

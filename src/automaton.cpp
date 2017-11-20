#include "dsp/digital.hpp"
#include "qwelk.hpp"

struct ModuleAutomaton : Module {
    enum ParamIds {
        PARAM_LIGHT,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightIds {
        LIGHT_MUTE,
        NUM_LIGHTS
    };

    int state = 0;
    SchmittTrigger trig;
    
    ModuleAutomaton() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step override;
}

void ModuleAutomaton::step() {
    if (trig.process(params[PARAM_LIGHT].value))
        state ^= 1;
    lights[LIGHT_MUTE].setBrightness(state ? 0.9 : 0.0);
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

    box.size(Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT));

    setPanel(SVG::load(assetPlugin(plugin, "res/Automaton.svg")));

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));
    
        addParam(createParam<Davies1900hBlackKnob>(Vec(28, 87), module, MyModule::PITCH_PARAM, -3.0, 3.0, 0.0));

    addInput(createInput<PJ301MPort>(Vec(33, 186), module, MyModule::PITCH_INPUT));

    addOutput(createOutput<PJ301MPort>(Vec(33, 275), module, MyModule::SINE_OUTPUT));

    addChild(createLight<MediumLight<RedLight>>(Vec(41, 59), module, MyModule::BLINK_LIGHT));

    addParam(createParam<LEDBezel>(Vec(50, 60), module, PARAM_LIGHT, 0.0, 1.0, 0.0));
    addChild(createLight<MuteLight<GreenLight>>(Vec(50, 60), module, LIGHT_MUTE));
}


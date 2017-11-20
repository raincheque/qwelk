#include "dsp/digital.hpp"
#include "qwelk.hpp"

struct ModuleAutomaton : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    ModuleAutomaton() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step override;
}

void ModuleAutomaton::step() {
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

    //add children
}


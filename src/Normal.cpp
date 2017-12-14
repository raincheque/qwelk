#include "dsp/digital.hpp"
#include "math.hpp"
#include "qwelk.hpp"

#define CHANNELS    8
#define WINDOW      16


struct ModuleNormal : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        IN_CHANNEL,
        NUM_INPUTS = IN_CHANNEL + CHANNELS
    };
    enum OutputIds {
        OUT_NORMAL,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    float psum = 0.0;
    float amps[CHANNELS] {};
    float window[CHANNELS * WINDOW] {};

    ModuleNormal() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

void ModuleNormal::step() {

    // 1. calc amp
    // 2. compensate gain
    
    const float brate = 1;
    float dt = 1.0 / engineGetSampleRate();
    
    float sum = 0.0;
    for (int i = 0; i < CHANNELS; ++i) {
        sum += inputs[IN_CHANNEL + i].value;
    }
    
    float avg = sum / (float)CHANNELS;
    outputs[OUT_NORMAL].value = avg;
}


WidgetNormal::WidgetNormal() {
    ModuleNormal *module = new ModuleNormal();
    setModule(module);

    box.size = Vec(2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Or.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));

    float x = box.size.x / 2.0 - 12, ytop = 45, ystep = 32.85;
    for (int i = 0; i < CHANNELS; ++i)
        addInput(createInput<PJ301MPort>(Vec(x, ytop + ystep * i), module, ModuleNormal::IN_CHANNEL + i));
    ytop += 9;
    addOutput(createOutput<PJ301MPort>( Vec(x, ytop + ystep * CHANNELS), module, ModuleNormal::OUT_NORMAL));
}


















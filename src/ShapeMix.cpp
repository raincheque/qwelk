#if 0

#include "dsp/digital.hpp"
#include "math.hpp"
#include "qwelk.hpp"

#define CHANNELS 8


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

    float amps[CHANNELS] {};
    float psum = 0.0;

    ModuleNormal() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

void ModuleNormal::step() {
    const float brate = 1;
    float dt = 1.0 / engineGetSampleRate();
    
    float sum = 0.0, max = 0.0;
    for (int i = 0; i < CHANNELS; ++i) {
        amps[i] = inputs[IN_CHANNEL + i].value;
        sum += amps[i];
        if (max < fabs(amps[i]))
            max = fabs(amps[i]);
    }
    
    float avg = sum / (float)CHANNELS;
    float amp = 1;
    outputs[OUT_NORMAL].value = sum / max;//avg * amp;
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

#endif

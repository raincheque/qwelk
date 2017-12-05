#include "dsp/digital.hpp"
#include "math.hpp"
#include "util.hpp"
#include "qwelk.hpp"


#define COMPONENTS 8


struct ModuleIndra : Module {
    enum ParamIds {
        PARAM_LFO,
        PARAM_PITCH,
        PARAM_SPREAD,
        PARAM_CFM, 
        PARAM_AMP = PARAM_CFM + COMPONENTS,
        PARAM_AMPSLEW = PARAM_AMP + COMPONENTS,
        PARAM_PHASESLEW = PARAM_AMPSLEW + COMPONENTS,
		NUM_PARAMS = PARAM_PHASESLEW + COMPONENTS
	};
	enum InputIds {
        IN_PITCH,
        IN_PHASE,
        IN_AMP = IN_PHASE + COMPONENTS,
        IN_CFM = IN_AMP + COMPONENTS,
        IN_RESET = IN_CFM + COMPONENTS,
		NUM_INPUTS,
	};
	enum OutputIds {
        OUT_COMPONENT,
		NUM_OUTPUTS = OUT_COMPONENT + COMPONENTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    SchmittTrigger trig_reset;
    float amp[COMPONENTS] {};
    float offset[COMPONENTS] {};
    float phase[COMPONENTS] {};
    

    ModuleIndra() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS)
    {
        for (int i = 0; i < COMPONENTS; ++i)
            amp[i] = 1.0;
    }
    void step() override;
};

static void _slew(float *v, float i, float sa, float min, float max)
{
    float shape = 0.25;
    if (i > *v) {
        float s = max * powf(min / max, sa);
        *v += s * crossf(1.0, (1/10.0) * (i - *v), shape) / engineGetSampleRate();
        if (*v > i)
            *v = i;
    } else if (i < *v) {
        float s = max * powf(min / max, sa);
        *v -= s * crossf(1.0, (1/10.0) * (*v - i), shape) / engineGetSampleRate();
        if (*v < i)
            *v = i;
    }
}

void ModuleIndra::step()
{
    const float slew_min = 0.1;
    const float slew_max = 1000.0;

    bool reset = trig_reset.process(inputs[IN_RESET].value);
    
    float spread = params[PARAM_SPREAD].value;
    
    float k = params[PARAM_PITCH].value;
    if (params[PARAM_LFO].value > 0.0)
        k = 54.0 * k;
    else
        k = 128.0 * k;
    float p = k + 12.0 * inputs[IN_PITCH].value;
    
    for (int i = 0; i < COMPONENTS; ++i) {
        
        if (inputs[IN_PHASE + i].active) {
            float sa = params[PARAM_PHASESLEW + i].value;
            float ip = clampf(inputs[IN_PHASE + i].value, 0, 10.0) / 10.0;
            _slew(offset + i, ip, sa, slew_min, slew_max);
        }

        float a = amp[i];
        a += params[PARAM_AMP + i].value;
        if (inputs[IN_AMP + i].active) {
            float sa = params[PARAM_AMPSLEW + i].value;
            float ia = clampf(inputs[IN_AMP + i].value, 0, 10.0) / 10.0;
            ia = ia * (1.0 - params[PARAM_AMP + i].value);
            _slew(amp + i, ia, sa, slew_min, slew_max);
        } else {
            a = params[PARAM_AMP + i].value;
        }

        if (inputs[IN_CFM + i].active)
            p += quadraticBipolar(params[PARAM_CFM + i].value) * 12.0 * inputs[IN_CFM + i].value;
        
        float f = 261.626 * powf(2.0, p / 12.0) * (i * spread + 1);
        phase[i] += f * (1.0 / engineGetSampleRate());
        while (phase[i] > 1.0)
            phase[i] -= 1.0;

        if (reset)
            phase[i] = offset[i];

        float o = offset[i];
        float v = sinf(2 * M_PI * (phase[i] + o));
        
        outputs[OUT_COMPONENT + i].value = a * 5.0 * v;
    }
}

struct RoundTinyKnob : RoundBlackKnob {
	RoundTinyKnob()
    {
		box.size = Vec(20, 20);
	}
};

WidgetIndra::WidgetIndra() {
    ModuleIndra *module = new ModuleIndra();
    setModule(module);

    box.size = Vec(16 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Blank_16.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

    float x = 2.5, y = 30, top = 0;

    addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleIndra::IN_PITCH));
    addInput(createInput<PJ301MPort>(Vec(x + 90, y), module, ModuleIndra::IN_RESET));
    addParam(createParam<RoundTinyKnob>(Vec(x + 30, y), module, ModuleIndra::PARAM_PITCH, -1.0, 1.0, 0.0));
    addParam(createParam<RoundTinyKnob>(Vec(x + 120, y), module, ModuleIndra::PARAM_SPREAD, 0.0, 1.0, 1.0));
    addParam(createParam<CKSS>(Vec(x + 60, y), module, ModuleIndra::PARAM_LFO, 0.0, 1.0, 0.0));

    x = x + 27;
    for (int i = 0; i < COMPONENTS; ++i) {
        y = top + 60;
        x = 2 + 30 * i;
        addParam(createParam<RoundTinyKnob>(Vec(x, y), module, ModuleIndra::PARAM_CFM + i, 0, 1, 0));
        y += 25;
        addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleIndra::IN_CFM + i));
        y += 30;
        addParam(createParam<RoundTinyKnob>(Vec(x, y), module, ModuleIndra::PARAM_PHASESLEW + i, 0, 1, 0));
        y += 25;
        addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleIndra::IN_PHASE + i));
        y += 30;
        addParam(createParam<BefacoSlidePot>(Vec(x, y), module, ModuleIndra::PARAM_AMP + i, 0, 1, 1));
        y += 115;
        addParam(createParam<RoundTinyKnob>(Vec(x, y), module, ModuleIndra::PARAM_AMPSLEW + i, 0, 1, 0));
        y += 25;
        addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleIndra::IN_AMP + i));
        y += 30;
        addOutput(createOutput<PJ301MPort>(Vec(x, y), module, ModuleIndra::OUT_COMPONENT + i));
    }
}

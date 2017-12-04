#include "dsp/decimator.hpp"
#include "dsp/filter.hpp"
#include "math.hpp"
#include "util.hpp"
#include "qwelk.hpp"


#define COMPONENTS 8


struct ModuleIndra : Module {
    enum ParamIds {
        PARAM_LFO,
        PARAM_PITCH,
        PARAM_AMPSLEW,
        PARAM_PHASESLEW = PARAM_AMPSLEW + COMPONENTS,
		NUM_PARAMS = PARAM_PHASESLEW + COMPONENTS,
	};
	enum InputIds {
        IN_PITCH,
        IN_PHASE,
        IN_AMP = IN_PHASE + COMPONENTS,
		NUM_INPUTS = IN_AMP + COMPONENTS,
	};
	enum OutputIds {
        OUT_COMPONENT,
		NUM_OUTPUTS = OUT_COMPONENT + COMPONENTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

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

    float k = params[PARAM_PITCH].value;
    if (params[PARAM_LFO].value > 0.0)
        k = 54.0 * k;
    else
        k = 128.0 * k;
    float p = k + inputs[IN_PITCH].value;
    float f = 261.626 * powf(2.0, p / 12.0);
    for (int i = 0; i < COMPONENTS; ++i) {
        
        if (inputs[IN_PHASE + i].active) {
            float sa = params[PARAM_PHASESLEW + i].value;
            float ip = clampf(inputs[IN_PHASE + i].value, 0, 10.0) / 10.0;
            _slew(offset + i, ip, sa, slew_min, slew_max);
        }

        float a = amp[i];
        if (inputs[IN_AMP + i].active) {
            float sa = params[PARAM_AMPSLEW + i].value;
            float ia = clampf(inputs[IN_AMP + i].value, 0, 10.0) / 10.0;

            /*/
            float shape = 0.25;
            if (ia > amp[i]) {
                float s = slew_max * powf(slew_min / slew_max, sa);
                amp[i] += s * crossf(1.0, (1/10.0) * (ia - amp[i]), shape) / engineGetSampleRate();
                if (amp[i] > ia)
                    amp[i] = ia;
            } else if (ia < amp[i]) {
                float s = slew_max * powf(slew_min / slew_max, sa);
                amp[i] -= s * crossf(1.0, (1/10.0) * (amp[i] - ia), shape) / engineGetSampleRate();
                if (amp[i] < ia)
                    amp[i] = ia;
            }
            /*/
            _slew(amp + i, ia, sa, slew_min, slew_max);
            //*/
        }

        phase[i] += (f * (i + 1)) * (1.0 / engineGetSampleRate());
        while (phase[i] > 1.0)
            phase[i] -= 1.0;

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
#define KNOB_RANGE 1.0
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

    float x = 2.5, y = 30, top = 30;
    
    addParam(createParam<CKSS>(Vec(x + 30, y), module, ModuleIndra::PARAM_LFO, 0.0, 1.0, 0.0));
    addParam(createParam<RoundTinyKnob>(Vec(x, y), module, ModuleIndra::PARAM_PITCH, -KNOB_RANGE, KNOB_RANGE, 0.0));
    addInput(createInput<PJ301MPort>(Vec(x, top + y), module, ModuleIndra::IN_PITCH));

    
    for (int i = 0; i < COMPONENTS; ++i) {
        addParam(createParam<RoundTinyKnob>(Vec(x + 30 * i, top + y * 2), module, ModuleIndra::PARAM_PHASESLEW + i, 0, 1, 0));        
        addInput(createInput<PJ301MPort>(Vec(x + 30 * i, top + y * 3), module, ModuleIndra::IN_PHASE + i));
        addParam(createParam<RoundTinyKnob>(Vec(x + 30 * i, top + y * 4), module, ModuleIndra::PARAM_AMPSLEW + i, 0, 1, 0));
        addInput(createInput<PJ301MPort>(Vec(x + 30 * i, top + y * 5), module, ModuleIndra::IN_AMP + i));
        addOutput(createOutput<PJ301MPort>(Vec(x + 30 * i, top + y * 6), module, ModuleIndra::OUT_COMPONENT + i));
    }
}

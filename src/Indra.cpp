#include "dsp/digital.hpp"
#include "math.hpp"
#include "util.hpp"
#include "qwelk.hpp"


#define COMPONENTS 8


struct ModuleIndra : Module {
    enum ParamIds {
        PARAM_CLEAN,
        PARAM_PITCH,
        PARAM_FM,
        PARAM_SPREAD,
        PARAM_CFM, 
        PARAM_AMP = PARAM_CFM + COMPONENTS,
        PARAM_AMPSLEW = PARAM_AMP + COMPONENTS,
        PARAM_PHASESLEW = PARAM_AMPSLEW + COMPONENTS,
		NUM_PARAMS = PARAM_PHASESLEW + COMPONENTS
	};
	enum InputIds {
        IN_PITCH,
        IN_FM,
        IN_SPREAD,
        IN_PHASE,
        IN_AMP = IN_PHASE + COMPONENTS,
        IN_CFM = IN_AMP + COMPONENTS,
        IN_RESET = IN_CFM + COMPONENTS,
		NUM_INPUTS,
	};
	enum OutputIds {
        OUT_COMPONENT,
        OUT_SUM = OUT_COMPONENT + COMPONENTS,
		NUM_OUTPUTS
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
    bool clean = params[PARAM_CLEAN].value > 0.0;
    
    float spread;
    if (inputs[IN_SPREAD].active)
        spread = (clampf(inputs[IN_SPREAD].value, 0.0, 10.0) / 10.0) * params[PARAM_SPREAD].value;
    else
        spread = params[PARAM_SPREAD].value;
    
    float k = params[PARAM_PITCH].value;
    float p = k + 12.0 * inputs[IN_PITCH].value;
    if (inputs[IN_FM].active)
        p += quadraticBipolar(params[PARAM_FM].value) * 12.0 * inputs[IN_FM].value;

    float tv = 0, ta = 0;
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

        ta += a;

        if (inputs[IN_CFM + i].active)
            p += quadraticBipolar(params[PARAM_CFM + i].value) * 12.0 * inputs[IN_CFM + i].value;
        
        float f = 261.626 * powf(2.0, p / 12.0) * (i * spread + 1);
        phase[i] += f * (1.0 / engineGetSampleRate());
        while (phase[i] > 1.0)
            phase[i] -= 1.0;

        float o = offset[i];
        
        if (reset)
            phase[i] = o;

        float p = 0.0;
        if (!clean)
            p = (1 - spread) * phase[0] + phase[i] * spread;
        else {
            p = phase[i];
            _slew(&p, (1 - spread) * phase[0] + phase[i] * spread, spread, slew_min, slew_max);
            phase[i] = p;
        }

        float v = sinf(2 * M_PI * (p + o));
        float r = a * 5.0 * v;
        outputs[OUT_COMPONENT + i].value = v * 5.0;
        tv += a * v;
    }

    outputs[OUT_SUM].value = (ta > 0 ? tv / ta : 0) * 5.0;
}

struct RoundTinyKnob : RoundBlackKnob {
	RoundTinyKnob()
    {
		box.size = Vec(20, 20);
	}
};

struct SlidePot : SVGSlider {
	SlidePot() {
        const float _h = 60;
		Vec margin = Vec(2.5, 2.5);
		maxHandlePos = Vec(-1, -2).plus(margin);
		minHandlePos = Vec(-1, _h-20).plus(margin);
		background->svg = SVG::load(assetPlugin(plugin, "res/SlidePot.svg"));
		background->wrap();
        background->box.size = Vec(background->box.size.x, _h);
		background->box.pos = margin;
		box.size = background->box.size.plus(margin.mult(2));
		handle->svg = SVG::load(assetPlugin(plugin, "res/SlidePotHandle.svg"));
		handle->wrap();
	}
};

WidgetIndra::WidgetIndra() {
    ModuleIndra *module = new ModuleIndra();
    setModule(module);

    box.size = Vec(16 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Indra.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(10, 0)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 20, 0)));
    addChild(createScrew<ScrewSilver>(Vec(10, 365)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 20, 365)));

    const float knob_x = 3;
    float x = 2.5, y = 42, top = 0;

    addInput(createInput<PJ301MPort>    (Vec(x,              y), module, ModuleIndra::IN_PITCH));
    addParam(createParam<RoundTinyKnob> (Vec(x + knob_x, y - 22), module, ModuleIndra::PARAM_PITCH, -54.0, 54.0, 0.0));
    
    addInput(createInput<PJ301MPort>    (Vec(x +  50,        y), module, ModuleIndra::IN_FM));
    addParam(createParam<RoundTinyKnob> (Vec(x +  50 + knob_x,   y - 22), module, ModuleIndra::PARAM_FM, 0.0, 1.0, 0.0));
    
    addInput(createInput<PJ301MPort>    (Vec(x +  105,        y), module, ModuleIndra::IN_RESET));

    addInput(createInput<PJ301MPort>    (Vec(x +  157,        y), module, ModuleIndra::IN_SPREAD));
    addParam(createParam<RoundTinyKnob> (Vec(x +  157 + knob_x,   y - 22), module, ModuleIndra::PARAM_SPREAD, 0.0, 1.0, 1.0));
    addParam(createParam<CKSS>          (Vec(x +  205,   y - 22), module, ModuleIndra::PARAM_CLEAN, 0.0, 1.0, 1.0));

    auto sum_pos = Vec(box.size.x / 2 - 12.5, 350);
    addOutput(createOutput<PJ301MPort>(sum_pos, module, ModuleIndra::OUT_SUM));

    x = x + 30;
    for (int i = 0; i < COMPONENTS; ++i) {
        y = top + 80;
        x = 2 + 30 * i;
        addParam(createParam<RoundTinyKnob>(Vec(x + knob_x, y), module, ModuleIndra::PARAM_CFM + i, 0, 1, 0));
        y += 22;
        addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleIndra::IN_CFM + i));
        y += 38;
        
        addParam(createParam<RoundTinyKnob>(Vec(x + knob_x, y), module, ModuleIndra::PARAM_PHASESLEW + i, 0, 1, 0));
        y += 22;
        addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleIndra::IN_PHASE + i));
        y += 35;
        
        addParam(createParam<SlidePot>(Vec(x + 5, y), module, ModuleIndra::PARAM_AMP + i, 0, 1, 1));
        y += 63;
        addParam(createParam<RoundTinyKnob>(Vec(x + knob_x, y), module, ModuleIndra::PARAM_AMPSLEW + i, 0, 1, 0));
        y += 22;
        addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleIndra::IN_AMP + i));
        y += 30;
        
        addOutput(createOutput<PJ301MPort>(Vec(x, y), module, ModuleIndra::OUT_COMPONENT + i));
    }
}

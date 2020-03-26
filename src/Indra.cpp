#include "qwelk.hpp"
#include "qwelk_common.h"

#define COMPONENTS 8

struct ModuleIndra : Module {
    enum ParamIds {
        PARAM_CLEAN,
        PARAM_PITCH,
        PARAM_FM,
        PARAM_SPREAD,
        ENUMS(PARAM_CFM, COMPONENTS),
        ENUMS(PARAM_AMP, COMPONENTS),
        ENUMS(PARAM_AMPSLEW, COMPONENTS),
        ENUMS(PARAM_PHASESLEW, COMPONENTS),
		    NUM_PARAMS
	};
	enum InputIds {
        IN_PITCH,
        IN_FM,
        IN_SPREAD,
        IN_RESET,
        ENUMS(IN_AMP, COMPONENTS),
        ENUMS(IN_CFM, COMPONENTS),
        ENUMS(IN_PHASE, COMPONENTS),
    		NUM_INPUTS
	};
	enum OutputIds {
        OUT_SUM,
        ENUMS(OUT_COMPONENT, COMPONENTS),
		    NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    bool attenuate_component_outs = false;
    dsp::SchmittTrigger trig_reset;
    float amp[COMPONENTS] {};
    float offset[COMPONENTS] {};
    float phase[COMPONENTS] {};


    ModuleIndra() {
      config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
      configParam(ModuleIndra::PARAM_PITCH, -54.0, 54.0, 0.0, "");
      configParam(ModuleIndra::PARAM_FM, 0.0, 1.0, 0.0, "");
      configParam(ModuleIndra::PARAM_SPREAD, 0.0, 1.0, 1.0, "");
      configParam(ModuleIndra::PARAM_CLEAN, 0.0, 1.0, 1.0, "");
      for (int i = 0; i < COMPONENTS; ++i) {
        configParam(ModuleIndra::PARAM_CFM + i, 0, 1, 0, "");
        configParam(ModuleIndra::PARAM_PHASESLEW + i, 0, 1, 0, "");
        configParam(ModuleIndra::PARAM_AMP + i, 0, 1, 1, "");
        configParam(ModuleIndra::PARAM_AMPSLEW + i, 0, 1, 0, "");
        amp[i] = 1.0;
      }
    }

    void process(const ProcessArgs& args) override;
};

static void _slew(float *v, float i, float sa, float min, float max)
{
    float shape = 0.25f;
    if (i > *v) {
        float s = max * powf(min / max, sa);
        *v += s * crossfade(1.0, (1/10.0f) * (i - *v), shape) / APP->engine->getSampleRate();
        if (*v > i)
            *v = i;
    } else if (i < *v) {
        float s = max * powf(min / max, sa);
        *v -= s * crossfade(1.0, (1/10.0f) * (*v - i), shape) / APP->engine->getSampleRate();
        if (*v < i)
            *v = i;
    }
}

void ModuleIndra::process(const ProcessArgs& args) {
    const float slew_min = 0.1;
    const float slew_max = 1000.0;

    bool reset = trig_reset.process(inputs[IN_RESET].getVoltage());
    bool clean = params[PARAM_CLEAN].getValue() > 0.0f;

    float spread;
    if (inputs[IN_SPREAD].isConnected())
        spread = (clampSafe(inputs[IN_SPREAD].getVoltage(), 0.0f, 10.0f) / 10.0f) * params[PARAM_SPREAD].getValue();
    else
        spread = params[PARAM_SPREAD].getValue();

    float k = params[PARAM_PITCH].getValue();
    float p = k + 12.0 * inputs[IN_PITCH].getVoltage();
    if (inputs[IN_FM].isConnected())
        p += dsp::quadraticBipolar(params[PARAM_FM].getValue()) * 12.0f * inputs[IN_FM].getVoltage();

    float tv = 0, ta = 0, ma = 0;
    for (int i = 0; i < COMPONENTS; ++i) {

        if (inputs[IN_PHASE + i].getVoltage()) {
            float sa = params[PARAM_PHASESLEW + i].getValue();
            float ip = clampSafe(inputs[IN_PHASE + i].getVoltage(), 0.0f, 10.0f) / 10.0f;
            _slew(offset + i, ip, sa, slew_min, slew_max);
        }

        float a = amp[i];
        a += params[PARAM_AMP + i].getValue();
        if (inputs[IN_AMP + i].isConnected()) {
            float sa = params[PARAM_AMPSLEW + i].getValue();
            float ia = clampSafe(inputs[IN_AMP + i].getVoltage(), 0.0f, 10.0f) / 10.0f;
            ia = ia * (1.0 - params[PARAM_AMP + i].getValue());
            _slew(amp + i, ia, sa, slew_min, slew_max);

            //a = amp[i];

        } else {
            a = params[PARAM_AMP + i].getValue();
        }

        ta += a;
        if (ma < fabs(a))
            ma = fabs(a);

        if (inputs[IN_CFM + i].getVoltage())
            p += dsp::quadraticBipolar(params[PARAM_CFM + i].getValue()) * 12.0f * inputs[IN_CFM + i].getVoltage();

        float f = 261.626f * powf(2.0, p / 12.0) * (i * spread + 1);
        phase[i] += f * (1.0f / args.sampleRate);
        while (phase[i] > 1.0f)
            phase[i] -= 1.0f;

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
        outputs[OUT_COMPONENT + i].setVoltage(v * 5.0 * (attenuate_component_outs ? a : 1.0));
        tv += a * v;
    }

    outputs[OUT_SUM].setVoltage((ta > 0 ? tv / ta : 0) * 5.0f * ma);
}


struct SlidePot : SvgSlider {
	SlidePot() {
        const float _h = 60;
		Vec margin = Vec(2.5, 2.5);
		maxHandlePos = Vec(-1, -2).plus(margin);
		minHandlePos = Vec(-1, _h-20).plus(margin);
    background->svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/SlidePot.svg"));
		background->wrap();
        background->box.size = Vec(background->box.size.x, _h);
		background->box.pos = margin;
		box.size = background->box.size.plus(margin.mult(2));
    handle->svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/SlidePotHandle.svg"));
		handle->wrap();
	}
};

struct MenuItemAttenuateComponentOuts : MenuItem {
    ModuleIndra *indra;
    void onAction(const event::Action &e) override
    {
        indra->attenuate_component_outs ^= true;
    }
    void step () override
    {
        rightText = (indra->attenuate_component_outs) ? "✔" : "";
    }
};

struct WidgetIndra : ModuleWidget {
  WidgetIndra(ModuleIndra *module) {

    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Indra.svg")));
    addChild(createWidget<ScrewSilver>(Vec(10, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 20, 0)));
    addChild(createWidget<ScrewSilver>(Vec(10, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 20, 365)));

    const float knob_x = 3;
    float x = 2.5, y = 42, top = 0;

    addInput(createInput<PJ301MPort>(      Vec(                x,      y), module, ModuleIndra::IN_PITCH));
    addParam(createParam<TinyKnob> (Vec(       x + knob_x, y - 22), module, ModuleIndra::PARAM_PITCH));

    addInput(createInput<PJ301MPort>(      Vec(          x +  50,      y), module, ModuleIndra::IN_FM));
    addParam(createParam<TinyKnob> (Vec( x +  50 + knob_x, y - 22), module, ModuleIndra::PARAM_FM));

    addInput(createInput<PJ301MPort>(      Vec(         x +  105,      y), module, ModuleIndra::IN_RESET));

    addInput(createInput<PJ301MPort>(      Vec(         x +  157,      y), module, ModuleIndra::IN_SPREAD));
    addParam(createParam<TinyKnob> (Vec(x +  157 + knob_x, y - 22), module, ModuleIndra::PARAM_SPREAD));
    addParam(createParam<CKSS>(     Vec(         x +  205, y - 22), module, ModuleIndra::PARAM_CLEAN));

    auto sum_pos = Vec(box.size.x / 2 - 12.5, 350);
    addOutput(createOutput<PJ301MPort>(sum_pos, module, ModuleIndra::OUT_SUM));

    x = x + 30;
    for (int i = 0; i < COMPONENTS; ++i) {
      y = top + 80;
      x = 2 + 30 * i;
      addParam(createParam<TinyKnob>(Vec(x + knob_x, y), module, ModuleIndra::PARAM_CFM + i));
      y += 22;
      addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleIndra::IN_CFM + i));
      y += 38;

      addParam(createParam<TinyKnob>(Vec(x + knob_x, y), module, ModuleIndra::PARAM_PHASESLEW + i));
      y += 22;
      addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleIndra::IN_PHASE + i));
      y += 35;

      addParam(createParam<SlidePot>(Vec(x + 5, y), module, ModuleIndra::PARAM_AMP + i));
      y += 63;
      addParam(createParam<TinyKnob>(Vec(x + knob_x, y), module, ModuleIndra::PARAM_AMPSLEW + i));
      y += 22;
      addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleIndra::IN_AMP + i));
      y += 30;

      addOutput(createOutput<PJ301MPort>(Vec(x, y), module, ModuleIndra::OUT_COMPONENT + i));
    }
  }

  void appendContextMenu(Menu *menu) override {
    if (module) {
      ModuleIndra *indra = dynamic_cast<ModuleIndra *>(module);
      assert(indra);

      MenuLabel *spacer = new MenuLabel();
      menu->addChild(spacer);

      MenuItemAttenuateComponentOuts *item = new MenuItemAttenuateComponentOuts();
      item->text = "Attenuate Component Outs";
      item->indra = indra;
      menu->addChild(item);
    }
  }
};

Model *modelIndra = createModel<ModuleIndra, WidgetIndra>("Indra");

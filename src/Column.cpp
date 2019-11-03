#include "qwelk.hpp"

#define CHANNELS 4

struct ModuleColumn : Module {
    enum ParamIds {
        PARAM_MASTER,
        PARAM_AVG,
        PARAM_WEIGHTED,
        NUM_PARAMS
    };
    enum InputIds {
        IN_SIG,
        IN_UPSTREAM = IN_SIG + CHANNELS,
        NUM_INPUTS = IN_UPSTREAM + CHANNELS
    };
    enum OutputIds {
        OUT_SIDE,
        OUT_DOWNSTREAM = OUT_SIDE + CHANNELS,
        NUM_OUTPUTS = OUT_DOWNSTREAM + CHANNELS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    bool allow_neg_weights = false;

    ModuleColumn() {
		  config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
      configParam(ModuleColumn::PARAM_AVG, 0.0, 1.0, 1.0, "");
      configParam(ModuleColumn::PARAM_WEIGHTED, 0.0, 1.0, 1.0, "");
    }
    void process(const ProcessArgs& args) override;
};

void ModuleColumn::process(const ProcessArgs& args)
{
    bool avg        = params[PARAM_AVG].getValue() == 0.0;
    bool weighted   = params[PARAM_WEIGHTED].getValue() == 0.0;

    float total     = 0;
    float on_count  = 0;

    for (int i = 0; i < CHANNELS; ++i) {
        float in_upstream   = inputs[IN_UPSTREAM + i].getVoltage();
        float in_sig        = inputs[IN_SIG + i].getVoltage();

        // just forward the input signal as the side stream,
        // used for chaining multiple Columns together
        outputs[OUT_SIDE + i].setVoltage(in_sig);

        if (inputs[IN_UPSTREAM + i].isConnected()) {
            if (weighted)
                on_count += allow_neg_weights ? in_upstream : fabs(in_upstream);
            else if (in_upstream != 0.0)
                on_count += 1;
        }

        if (!weighted && in_sig != 0.0)
            on_count += 1;

        float product = weighted ? (in_upstream * in_sig) : (in_upstream + in_sig);

        total += product;

        outputs[OUT_DOWNSTREAM + i].setVoltage(avg ? ((on_count != 0) ? (total / on_count) : 0) : (total));
    }
}

struct MenuItemAllowNegWeights : MenuItem {
    ModuleColumn *col;
    void onAction(const event::Action &e) override
    {
        col->allow_neg_weights ^= true;
    }
    void step () override
    {
        rightText = (col->allow_neg_weights) ? "✔" : "";
    }
};

struct WidgetColumn : ModuleWidget {
  WidgetColumn(ModuleColumn *module) {

		setModule(module);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Column.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));

    addParam(createParam<CKSS>(Vec(3.5, 30), module, ModuleColumn::PARAM_AVG));
    addParam(createParam<CKSS>(Vec(42, 30), module, ModuleColumn::PARAM_WEIGHTED));

    float x = 2.5, xstep = 15, ystep = 23.5;
    for (int i = 0; i < CHANNELS; ++i)
    {
        float y = 80 + i * 80;
        addInput(createInput<PJ301MPort>(Vec(x + xstep, y - ystep), module, ModuleColumn::IN_UPSTREAM + i));
        addOutput(createOutput<PJ301MPort>(Vec(x + xstep*2, y), module, ModuleColumn::OUT_SIDE + i));
        addInput(createInput<PJ301MPort>(Vec(x, y), module, ModuleColumn::IN_SIG + i));
        addOutput(createOutput<PJ301MPort>(Vec(x + xstep, y + ystep), module, ModuleColumn::OUT_DOWNSTREAM + i));
    }
}

  void appendContextMenu(Menu *menu) override {
    if (module) {
      ModuleColumn *column = dynamic_cast<ModuleColumn *>(module);
      assert(column);

      MenuLabel *spacer = new MenuLabel();
      menu->addChild(spacer);

      MenuItemAllowNegWeights *item = new MenuItemAllowNegWeights();
      item->text = "Allow Negative Weights";
      item->col  = column;
      menu->addChild(item);
    }
  }
};

Model *modelColumn = createModel<ModuleColumn, WidgetColumn>("Column");

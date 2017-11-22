#include "dsp/digital.hpp"
#include "qwelk.hpp"


#define CHANNELS 8


struct ModuleAutomaton : Module {
    enum ParamIds {
        PARAM_SCAN,
        PARAM_STEP,
        PARAM_CELL,
        NUM_PARAMS = PARAM_CELL + CHANNELS * 2
    };
    enum InputIds {
        INPUT_SCAN,
        INPUT_STEP,
        INPUT_RULE,
        NUM_INPUTS = INPUT_RULE + 8
    };
    enum OutputIds {
        OUTPUT_COUNT,
        OUTPUT_NUMBER,
        OUTPUT_CELL,
        NUM_OUTPUTS = OUTPUT_CELL + CHANNELS
    };
    enum LightIds {
        LIGHT_POS_SCAN,
        LIGHT_NEG_SCAN,
        LIGHT_STEP,
        LIGHT_MUTE,
        NUM_LIGHTS = LIGHT_MUTE + CHANNELS * 2
    };

    int             scan = 1;
    SchmittTrigger  trig_step_input;
    SchmittTrigger  trig_step_manual;
    SchmittTrigger  trig_scan_manual;
    SchmittTrigger  trig_scan_input;
    SchmittTrigger  trig_cells[CHANNELS*2];
    int             states[CHANNELS*2] {};
    
    const float     output_volt = 5.0;
    const float     output_volt_uni = output_volt * 2;

    
    ModuleAutomaton() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

void ModuleAutomaton::step()
{
    int nextstep = 0;
    if (trig_step_manual.process(params[PARAM_STEP].value)
        || trig_step_input.process(inputs[INPUT_STEP].value))
        nextstep = 1;

    // determine scan direction
    if (trig_scan_input.process(inputs[INPUT_SCAN].value))
        scan = inputs[INPUT_SCAN].normalize(scan) < 0 ? -1 : 1;
    // manual tinkering with step?
    if (trig_scan_manual.process(params[PARAM_SCAN].value))
        scan *= -1;
    
    if (nextstep) {
        int rule = 0;
        // read rule from inputs
        for (int i = 0; i < CHANNELS; ++i)
            if (inputs[INPUT_RULE + i].active && inputs[INPUT_RULE + i].value > 0.0)
                rule |= 1 << i;
        // copy prev state to output cells
        for (int i = 0; i < CHANNELS; ++i)
            states[CHANNELS + i] = states[i];
        // determine the next gen
        for (int i = 0; i < CHANNELS; ++i) {
            int sum = 0;
            int tl  = i == 0 ? CHANNELS - 1 : i - 1;
            int tm  = i;
            int tr  = i < CHANNELS - 1 ? i : 0;
            sum |= states[CHANNELS + tr] ? (1 << 0) : 0;
            sum |= states[CHANNELS + tm] ? (1 << 1) : 0;
            sum |= states[CHANNELS + tl] ? (1 << 2) : 0;
            states[i] = (rule & (1 << sum)) != 0;
        }
    }

    // handle manual tinkering with the state
    for (int i = 0; i < CHANNELS * 2; ++i)
        if (trig_cells[i].process(params[PARAM_CELL + i].value))
            states[i] ^= 1;
    
    int count = 0, number = 0;
    for (int i = 0; i < CHANNELS; ++i) {
        count += states[i + CHANNELS];
        if (scan >= 0)
            number |= ((1 << i) * states[CHANNELS + i]);
        else
            number |= ((1 << (CHANNELS - 1 - i)) * states[CHANNELS + i]);
    }

    // individual gate output
    for (int i = 0; i < CHANNELS; ++i)
        outputs[OUTPUT_CELL + i].value = states[i + CHANNELS] ? output_volt : 0.0;
    // number of LIVE cells
    outputs[OUTPUT_COUNT].value = ((float)count / (float)CHANNELS) * output_volt_uni;
    // the binary number LIVE cells represent 
    outputs[OUTPUT_NUMBER].value = ((float)number / (float)(1 << (CHANNELS))) * 10.0;

    // indicate step direction
    lights[LIGHT_POS_SCAN].setBrightness(scan < 0 ? 0.0 : 0.9);
    lights[LIGHT_NEG_SCAN].setBrightness(scan < 0 ? 0.9 : 0.0);
    // indicate next generation
    lights[LIGHT_STEP].setBrightness(trig_step_manual.isHigh() || trig_step_input.isHigh() ? 0.9 : 0.0);
    // blink according to state
    for (int i = 0; i < CHANNELS * 2; ++i)
        lights[LIGHT_MUTE + i].setBrightness(states[i] ? 0.9 : 0.0);
}



template <typename _BASE>
struct MuteLight : _BASE {
    MuteLight()
    {
        this->box.size = mm2px(Vec(6, 6));
    }
};


WidgetAutomaton::WidgetAutomaton()
{
    ModuleAutomaton *module = new ModuleAutomaton();
    setModule(module);

    box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Automaton.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));
 
    const float ypad = 27.5;
    const float tlpy = 1.75;
    const float lghx = box.size.x / 2.0;
    const float tlpx = 2.25;
    const float dist = 25;
    
    float ytop = 55;

    addInput(createInput<PJ301MPort>(               Vec(lghx - dist * 2      , ytop - ypad         ), module, ModuleAutomaton::INPUT_SCAN));
    addParam(createParam<LEDBezel>(                 Vec(lghx + dist          , ytop - ypad         ), module, ModuleAutomaton::PARAM_SCAN, 0.0, 1.0, 0.0));
    addChild(createLight<MuteLight<GreenRedLight>>( Vec(lghx + dist + tlpx   , ytop - ypad + tlpy  ), module, ModuleAutomaton::LIGHT_POS_SCAN));

    ytop += ypad;
    
    addInput(createInput<PJ301MPort>(           Vec(lghx - dist * 2     , ytop - ypad         ), module, ModuleAutomaton::INPUT_STEP));
    addParam(createParam<LEDBezel>(             Vec(lghx + dist         , ytop - ypad         ), module, ModuleAutomaton::PARAM_STEP, 0.0, 1.0, 0.0));
    addChild(createLight<MuteLight<GreenLight>>(Vec(lghx + dist + tlpx  , ytop - ypad + tlpy  ), module, ModuleAutomaton::LIGHT_STEP));
    
    for (int i = 0; i < CHANNELS; ++i) {
        addInput(createInput<PJ301MPort>(           Vec(lghx - dist * 2     , ytop + ypad * i       ), module, ModuleAutomaton::INPUT_RULE + i));
        addParam(createParam<LEDBezel>(             Vec(lghx - dist         , ytop + ypad * i       ), module, ModuleAutomaton::PARAM_CELL + i, 0.0, 1.0, 0.0));
        addChild(createLight<MuteLight<GreenLight>>(Vec(lghx - dist + tlpx  , ytop + ypad * i + tlpy), module, ModuleAutomaton::LIGHT_MUTE + i));
        addParam(createParam<LEDBezel>(             Vec(lghx                , ytop + ypad * i       ), module, ModuleAutomaton::PARAM_CELL + CHANNELS + i, 0.0, 1.0, 0.0));
        addChild(createLight<MuteLight<GreenLight>>(Vec(lghx + tlpx         , ytop + ypad * i + tlpy), module, ModuleAutomaton::LIGHT_MUTE + CHANNELS + i));
        addOutput(createOutput<PJ301MPort>(         Vec(lghx + dist         , ytop + ypad * i       ), module, ModuleAutomaton::OUTPUT_CELL + i));
    }
    
    const float output_y = ytop + ypad * CHANNELS;
    addOutput(createOutput<PJ301MPort>(Vec(lghx + dist, output_y        ), module, ModuleAutomaton::OUTPUT_NUMBER));
    addOutput(createOutput<PJ301MPort>(Vec(lghx + dist, output_y + ypad ), module, ModuleAutomaton::OUTPUT_COUNT));
}

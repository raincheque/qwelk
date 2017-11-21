#include "dsp/digital.hpp"
#include "qwelk.hpp"


#define FEEDBACK 1
#define CHANNELS 8


struct ModuleAutomaton : Module {
    enum ParamIds {
        PARAM_STEP,
        PARAM_CELL,
        NUM_PARAMS = PARAM_CELL + CHANNELS * 2
    };
    enum InputIds {
        INPUT_STEP,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT_ANY,
        OUTPUT_NUMON,
        OUTPUT_NUMBER,
        OUTPUT_CELL,
        NUM_OUTPUTS = OUTPUT_CELL + CHANNELS
    };
    enum LightIds {
        LIGHT_STEP,
        LIGHT_MUTE,
        NUM_LIGHTS = LIGHT_MUTE + CHANNELS * 2
    };

    int rule = 30;
    SchmittTrigger trig_step_input;
    SchmittTrigger trig_step_manual;
    SchmittTrigger trig_cells[CHANNELS*2];
    int states[CHANNELS*2] {};

    
    ModuleAutomaton() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

void ModuleAutomaton::step() {
    int nextstep = 0;
    
    if (trig_step_manual.process(params[PARAM_STEP].value))
        nextstep = 1;

    if (trig_step_input.process(inputs[INPUT_STEP].value))
        nextstep = 1;
    
    lights[LIGHT_STEP].setBrightness(trig_step_manual.isHigh() || trig_step_input.isHigh() ? 0.9 : 0.0);    

    if (nextstep) {
        for (int i = 0; i < CHANNELS; ++i) {
            states[CHANNELS + i] = states[i];
        }
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

    // handle manual input
    for (int i = 0; i < CHANNELS * 2; ++i)
        if (trig_cells[i].process(params[PARAM_CELL + i].value))
            states[i] ^= 1;
    
    for (int i = 0; i < CHANNELS * 2; ++i)
        lights[LIGHT_MUTE + i].setBrightness(states[i] ? 0.9 : 0.0);

    
    const float output_volt = 5.0;
    const float output_volt_uni = output_volt * 2;
    for (int i = 0; i < CHANNELS; ++i)
        outputs[OUTPUT_CELL + i].value = states[i + CHANNELS] ? output_volt : 0.0;

    int oncount = 0, number = 0;
    for (int i = 0; i < CHANNELS; ++i) {
        oncount += states[i + CHANNELS];
        number |= ((1 << i) * states[i + CHANNELS]);
    }
    
    outputs[OUTPUT_ANY].value = oncount ? output_volt : 0.0;
    outputs[OUTPUT_NUMON].value = ((float)oncount / (float)CHANNELS) * output_volt_uni;
    outputs[OUTPUT_NUMBER].value = ((float)number / (float)(1 << (CHANNELS))) * 10.0;
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

    box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
    // setPanel(SVG::load(assetPlugin(plugin, "res/Blank.svg")));
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
    
    // addParam(createParam<Davies1900hBlackKnob>(Vec(28, 87), module, MyModule::PITCH_PARAM, -3.0, 3.0, 0.0));
    // addChild(createLight<MediumLight<RedLight>>(Vec(41, 59), module, MyModule::BLINK_LIGHT));
    
    const float ypad = 33;
    const float ytop = 50;
    const float lghx = box.size.x / 2.0 - 10.0;
    const float tlpx = 2.25;
    const float tlpy = 1.75;
    const float dist = 24;

    // next step
    for (int i = 0; i < CHANNELS; ++i) {
        addParam(createParam<LEDBezel>(Vec(lghx - dist, ytop + ypad * i), module, ModuleAutomaton::PARAM_CELL + i, 0.0, 1.0, 0.0));
        addChild(createLight<MuteLight<GreenLight>>(Vec(lghx - dist + tlpx, ytop + tlpy + ypad * i), module, ModuleAutomaton::LIGHT_MUTE + i));
        
        addParam(createParam<LEDBezel>(Vec(lghx, ytop + ypad * i), module, ModuleAutomaton::PARAM_CELL + CHANNELS + i, 0.0, 1.0, 0.0));
        addChild(createLight<MuteLight<GreenLight>>(Vec(lghx + tlpx, ytop + tlpy + ypad * i), module, ModuleAutomaton::LIGHT_MUTE + CHANNELS + i));

        addOutput(createOutput<PJ301MPort>(Vec(lghx + 25, ytop + ypad * i), module, ModuleAutomaton::OUTPUT_CELL + i));
    }

    addParam(createParam<LEDBezel>(Vec(box.size.x / 3.0 - 35, ytop - 20), module, ModuleAutomaton::PARAM_STEP, 0.0, 1.0, 0.0));
    addChild(createLight<MuteLight<GreenLight>>(Vec(box.size.x / 3.0 - 35, ytop - 20), module, ModuleAutomaton::LIGHT_STEP));
    
    addInput(createInput<PJ301MPort>(Vec(box.size.x / 3.0 - 35, ytop), module, ModuleAutomaton::INPUT_STEP));
    
    addOutput(createOutput<PJ301MPort>(Vec(lghx - 25, 320), module, ModuleAutomaton::OUTPUT_NUMBER));
    addOutput(createOutput<PJ301MPort>(Vec(lghx, 320), module, ModuleAutomaton::OUTPUT_NUMON));
    addOutput(createOutput<PJ301MPort>(Vec(lghx + 25, 320), module, ModuleAutomaton::OUTPUT_ANY));
}

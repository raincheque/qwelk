#include "qwelk.hpp"
#include "qwelk_common.h"


#define GWIDTH          4
#define GHEIGHT         8
#define GSIZE           (GWIDTH*GHEIGHT)
#define GMID            (GWIDTH/2+(GHEIGHT/2)*GWIDTH)
#define LIGHT_SIZE      10
#define DIR_BIT_SIZE    8


struct ModuleNews : Module {
    enum ParamIds {
        PARAM_MODE,
        PARAM_GATEMODE,
        PARAM_ROUND,
        PARAM_CLAMP,
        PARAM_INTENSITY,
        PARAM_WRAP,
        PARAM_SMOOTH,
        PARAM_UNI_BI,
        PARAM_ORIGIN,
        NUM_PARAMS
    };
    enum InputIds {
        IN_NEWS,
        IN_INTENSITY,
        IN_WRAP,
        IN_HOLD,
        IN_ORIGIN,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_CELL,
        NUM_OUTPUTS = OUT_CELL + GSIZE
    };
    enum LightIds {
        LIGHT_GRID,
        NUM_LIGHTS = LIGHT_GRID + GSIZE
    };

    float sample;
    dsp::SchmittTrigger trig_hold;
    byte grid[GSIZE] {};
    float buffer[GSIZE] {};

  ModuleNews() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configParam(ModuleNews::PARAM_ORIGIN, 0.0, GSIZE, GMID, "");
    configParam(ModuleNews::PARAM_INTENSITY, 1.0, 256.0, 1.0, "");
    configParam(ModuleNews::PARAM_WRAP, -31.0, 32.0, 0.0, "");
    configParam(ModuleNews::PARAM_UNI_BI, 0.0, 1.0, 1.0, "");
    configParam(ModuleNews::PARAM_MODE, 0.0, 1.0, 1.0, "");
    configParam(ModuleNews::PARAM_GATEMODE, 0.0, 1.0, 1.0, "");
    configParam(ModuleNews::PARAM_ROUND, 0.0, 1.0, 1.0, "");
    configParam(ModuleNews::PARAM_CLAMP, 0.0, 1.0, 1.0, "");
    configParam(ModuleNews::PARAM_SMOOTH, 0.0, 1.0, 0.0, "");
  }
    void process(const ProcessArgs& args) override;

    inline void set(int i, bool gatemode)
    {
        if (gatemode)
            grid[i] ^= 1;
        else
            grid[i] += 1;
    }
    inline void set(int x, int y, bool gatemode)
    {
        set(x + y * GWIDTH, gatemode);
    }
};

void ModuleNews::process(const ProcessArgs& args)
{
    bool    mode        = params[PARAM_MODE].getValue() > 0.0;
    bool    gatemode    = params[PARAM_GATEMODE].getValue() > 0.0;
    bool    round       = params[PARAM_ROUND].getValue() == 0.0;
    bool    clamp       = params[PARAM_CLAMP].getValue() == 0.0;
    bool    bi          = params[PARAM_UNI_BI].getValue() == 0.0;
    byte    intensity   = (byte)(floor(params[PARAM_INTENSITY].getValue()));
    int     wrap        = floor(params[PARAM_WRAP].getValue());
    int     origin      = floor(params[PARAM_ORIGIN].getValue());
    float   smooth      = params[PARAM_SMOOTH].getValue();

    float in_origin     = inputs[IN_ORIGIN].getVoltage() / 10.0;
    float in_intensity  = (inputs[IN_INTENSITY].getVoltage() / 10.0) * 255.0;
    float in_wrap       = (inputs[IN_WRAP].getVoltage() / 5.0) * 31.0;


    intensity = minb(intensity + (byte)in_intensity, 255.0);
    wrap = ::clampi(wrap + (int)in_wrap, -31, 31);

    // are we doing s&h?
    if (trig_hold.process(inputs[IN_HOLD].getVoltage()))
        sample = inputs[IN_NEWS].getVoltage();

    // read the news, or if s&h is active just the held sample
    float news = (inputs[IN_HOLD].isConnected()) ? sample : inputs[IN_NEWS].getVoltage();

    // if round switch is down, round off the  input signal to an integer
    if (round)
        news = ceil(news);

    // wrap the bits around, e.g. wrap = 2, 1001 ->  0110 / wrap = -3,  1001 -> 0011
    uint32_t bits;
    memcpy(&bits, &news, sizeof news);
    if (wrap > 0)
        bits = (bits << wrap) | (bits >> (32 - wrap));
    else if (wrap < 0) {
        wrap = -wrap;
        bits = (bits >> wrap) | (bits << (32 - wrap));
    }

    memcpy(&news, &bits, sizeof bits);

    // extract the key out the bits which represent the input signal
    uint32_t key;
    memcpy(&key, &news, sizeof news);

    // reset grid
    for (int i = 0; i < GSIZE; ++i)
        grid[i] = 0;

    // determine origin
    origin = std::min(origin + (int)floor(in_origin * GSIZE), GSIZE );
    int cy = origin / GWIDTH,
        cx = origin % GWIDTH;

    // extract N-E-W-S steps
    int nort = (key >> 24) & 0xFF,
        east = (key >> 16) & 0xFF,
        sout = (key >>  8) & 0xFF,
        west = (key      ) & 0xFF;

    // begin plotting, or 'the walk'
    int w = 0,
       ic = (mode ? 1 : DIR_BIT_SIZE),
     cond = 0;
    while (w++ < ic) {
        cond = mode ? (nort) : (((nort >> w) & 1) == 1);
        while (cond-- > 0) {
            cy = (cy - 1) >= 0 ? cy - 1 : GHEIGHT - 1;
            set(cx, cy, gatemode);
        }
        cond = (mode ? (east) : (((east >> w) & 1) == 1));
        while (cond-- > 0) {
            cx = (cx + 1) < GWIDTH ? cx + 1 : 0;
            set(cx, cy, gatemode);
        }
        cond = (mode ? (sout) : (((sout >> w) & 1) == 1));
        while (cond-- > 0) {
            cy = (cy + 1) < GHEIGHT ? cy + 1 : 0;
            set(cx, cy, gatemode);
        }
        cond = (mode ? (west) : (((west >> w) & 1) == 1));
        while (cond-- > 0) {
            cx = (cx - 1) >= 0 ? cx - 1 : GWIDTH - 1;
            set(cx, cy, gatemode);
        }
    }

    // output
    for (int i = 0; i < GSIZE; ++i) {
        byte r = grid[i] * intensity;
        if (clamp && ((int)grid[i] * (int)intensity) > 0xFF)
            r = 0xFF;

        float v = gatemode
                  ? (grid[i] ? 1 : 0)
                  : ((byte)r / 255.0);

        buffer[i] = slew(buffer[i], v, smooth, 0.1, 100000.0, 0.5);

        outputs[OUT_CELL + i].setVoltage(10.0 * buffer[i] - (bi ? 5.0 : 0.0));
        lights[LIGHT_GRID + i].setBrightness(buffer[i] * 0.9);
    }
}

//TODO: move to common
template <typename _BASE>
struct CellLight : _BASE {
    CellLight()
    {
        this->box.size = mm2px(Vec(LIGHT_SIZE, LIGHT_SIZE));
    }
};

struct WidgetNews : ModuleWidget {
  WidgetNews(ModuleNews *module) {
		setModule(module);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/NEWS.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));


    addInput(createInput<PJ301MPort>(Vec(9 , 30), module, ModuleNews::IN_HOLD));
    addInput(createInput<PJ301MPort>(Vec(9 , 65), module, ModuleNews::IN_NEWS));

    addInput(createInput<PJ301MPort>(Vec(39 , 65), module, ModuleNews::IN_ORIGIN));
    addParam(createParam<TinyKnob>(Vec(41.6, 32.5), module, ModuleNews::PARAM_ORIGIN));
    addInput(createInput<PJ301MPort>(Vec(69 , 65), module, ModuleNews::IN_INTENSITY));
    addParam(createParam<TinyKnob>(Vec(71.1 , 32.5), module, ModuleNews::PARAM_INTENSITY));
    addInput(createInput<PJ301MPort>(Vec(99, 65), module, ModuleNews::IN_WRAP));
    addParam(createParam<TinyKnob>(Vec(101, 32.5), module, ModuleNews::PARAM_WRAP));

    const float out_ytop = 92.5;
    for (int y = 0; y < GHEIGHT; ++y)
        for (int x = 0; x < GWIDTH; ++x) {
            int i = x + y * GWIDTH;
            addChild(createLight<CellLight<GreenLight>>(Vec(7 + x * 30 - 0.2, out_ytop + y * 30 - 0.1), module, ModuleNews::LIGHT_GRID + i));
            addOutput(createOutput<PJ301MPort>(Vec(7 + x * 30 + 2, out_ytop + y * 30 + 2), module, ModuleNews::OUT_CELL + i));
        }

    const float bottom_row = 345;
    addParam(createParam<CKSS>(Vec(5        , bottom_row), module, ModuleNews::PARAM_UNI_BI));
    addParam(createParam<CKSS>(Vec(25       , bottom_row), module, ModuleNews::PARAM_MODE));
    addParam(createParam<CKSS>(Vec(45       , bottom_row), module, ModuleNews::PARAM_GATEMODE));
    addParam(createParam<CKSS>(Vec(65       , bottom_row), module, ModuleNews::PARAM_ROUND));
    addParam(createParam<CKSS>(Vec(85       , bottom_row), module, ModuleNews::PARAM_CLAMP));
    addParam(createParam<TinyKnob>(Vec(110  , bottom_row), module, ModuleNews::PARAM_SMOOTH));
  }
};

Model *modelNews = createModel<ModuleNews, WidgetNews>("NEWS");

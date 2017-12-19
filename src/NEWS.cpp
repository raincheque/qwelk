#include "dsp/digital.hpp"
#include "math.hpp"
#include "qwelk.hpp"


#define GWIDTH  4
#define GHEIGHT 8
#define LIGHT_SIZE 10

typedef unsigned char byte;


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
        PARAM_OFFSET,
        NUM_PARAMS
    };
    enum InputIds {
        IN_NEWS,
        IN_INTENSITY,
        IN_WRAP,
        IN_HOLD,
        IN_OFFSET,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_CELL,
        NUM_OUTPUTS = OUT_CELL + GWIDTH * GHEIGHT
    };
    enum LightIds {
        LIGHT_GRID,
        NUM_LIGHTS = LIGHT_GRID + GWIDTH * GHEIGHT
    };

    float sample;
    SchmittTrigger trig_hold;
    byte grid[GWIDTH * GHEIGHT] {};
    float buffer[GWIDTH * GHEIGHT] {};

    ModuleNews() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

byte minb(byte a, byte b) {return a < b ? a : b;}
byte maxb(byte a, byte b) {return a > b ? a : b;}
int clampfuck(int v, int l, int h) {return (v < l) ? l : ((v > h) ? h : v);}
void _slew(float *v, float i, float sa, float min, float max)
{
    float shape = 0.5;
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

void ModuleNews::step()
{
    bool    mode        = params[PARAM_MODE].value > 0.0;
    bool    gatemode    = params[PARAM_GATEMODE].value > 0.0;
    bool    round       = params[PARAM_ROUND].value == 0.0;
    bool    clamp       = params[PARAM_CLAMP].value == 0.0;
    bool    bi          = params[PARAM_UNI_BI].value == 0.0;
    byte    intensity   = (byte)(floor(params[PARAM_INTENSITY].value));
    int     wrap        = (int)(floor(params[PARAM_WRAP].value));
    float   smooth      = params[PARAM_SMOOTH].value;
    
    float in_intensity  = (inputs[IN_INTENSITY].value / 10.0) * 255.0;
    float in_wrap       = (inputs[IN_WRAP].value / 5.0) * 31.0;

    intensity = minb(intensity + (byte)in_intensity, 255.0);
    wrap = clampfuck(wrap + (int)in_wrap, -31, 31);

    if (trig_hold.process(inputs[IN_HOLD].value))
        sample = inputs[IN_NEWS].value;
    
    float news = (inputs[IN_HOLD].active) ?  sample : inputs[IN_NEWS].value;

    if (round)
        news = (int)news;
        
    unsigned bits = *(reinterpret_cast<unsigned *>(&news));
    if (wrap > 0)
        bits = (bits << wrap) | (bits >> (32 - wrap));
    else if (wrap < 0)
        bits = (bits >> wrap) | (bits << (32 - wrap));
    
    news = *((float *)&bits);

    unsigned key = *(reinterpret_cast<unsigned *>(&news));

    // reset grid
    for (int i = 0; i < GWIDTH * GHEIGHT; ++i)
        grid[i] = 0;

    // extract N-E-W-S info
    int up = (key >> 24) & 0xFF;
    int rt = (key >> 16) & 0xFF;
    int dn = (key >>  8) & 0xFF;
    int lt = (key      ) & 0xFF;

    // read the N-E-W-S
    const int max_offset = GWIDTH * GHEIGHT;
    int offset = floor(params[PARAM_OFFSET].value);
    offset = maxi(offset + floor((inputs[IN_OFFSET].value / 10.0) * max_offset), max_offset);
    int cy = offset / GWIDTH,  // GHEIGHT / 2,
        cx = offset % GWIDTH;  // GWIDTH / 2;

    int w = 0;
    while (w++ < (mode ? 1 : 8)) {
        int cond = mode ? (up) : (((up >> w) & 1) == 1);
        while (cond-- > 0) {
            cy = (cy - 1) >= 0 ? cy - 1 : GHEIGHT - 1;
            if (gatemode)
                grid[cx + cy * GWIDTH] ^= 1;
            else
                grid[cx + cy * GWIDTH] += 1;
        }
        cond = (mode ? (rt) : (((rt >> w) & 1) == 1));
        while (cond-- > 0) {
            cx = (cx + 1) < GWIDTH ? cx + 1 : 0;
            if (gatemode)
                grid[cx + cy * GWIDTH] ^= 1;
            else
                grid[cx + cy * GWIDTH] += 1;
        }
        cond = (mode ? (dn) : (((dn >> w) & 1) == 1));
        while (cond-- > 0) {
            cy = (cy + 1) < GHEIGHT ? cy + 1 : 0;
            if (gatemode)
                grid[cx + cy * GWIDTH] ^= 1;
            else
                grid[cx + cy * GWIDTH] += 1;
        }
        cond = (mode ? (lt) : (((lt >> w) & 1) == 1));
        while (cond-- > 0) {
            cx = (cx - 1) >= 0 ? cx - 1 : GWIDTH - 1;
            if (gatemode)
                grid[cx + cy * GWIDTH] ^= 1;
            else
                grid[cx + cy * GWIDTH] += 1;
        }
    }

    // output
    for (int y = 0; y < GHEIGHT; ++y)
        for (int x = 0; x < GWIDTH; ++x) {
            int i = x + y * GWIDTH;
            
            byte r = grid[i] * intensity;
            if (clamp && ((int)grid[i] * intensity) > 0xFF)
                r = 0xFF;

            float v = gatemode
                      ? (grid[i] ? 1 : 0)
                      : ((byte)r / 255.0);

            float l = v * 0.9;

            _slew(buffer + i, v, smooth, 0.1, 100000.0);

            outputs[OUT_CELL + i].value = 10.0 * buffer[i] - (bi ? 5.0 : 0.0);
            lights[LIGHT_GRID + i].setBrightness(buffer[i] * 0.9);

            //buffer[i] = v;
        }

}

struct TinyKnob : RoundBlackKnob {
	TinyKnob()
    {
		box.size = Vec(20, 20);
	}
};
template <typename _BASE>
struct CellLight : _BASE {
    CellLight()
    {
        this->box.size = mm2px(Vec(LIGHT_SIZE, LIGHT_SIZE));
    }
};

WidgetNews::WidgetNews()
{
    ModuleNews *module = new ModuleNews();
    setModule(module);

    box.size = Vec(9 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
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


    addInput(createInput<PJ301MPort>(Vec(10 , 30), module, ModuleNews::IN_HOLD));
    addInput(createInput<PJ301MPort>(Vec(10 , 60), module, ModuleNews::IN_NEWS));
    addInput(createInput<PJ301MPort>(Vec(40 , 30), module, ModuleNews::IN_OFFSET));
    addParam(createParam<TinyKnob>(Vec(45   , 60), module, ModuleNews::PARAM_OFFSET, 0.0, GWIDTH*GHEIGHT, (GWIDTH/2+(GHEIGHT/2)*GWIDTH)));
    addInput(createInput<PJ301MPort>(Vec(70 , 30), module, ModuleNews::IN_INTENSITY));
    addParam(createParam<TinyKnob>(Vec(80   , 60), module, ModuleNews::PARAM_INTENSITY, 1.0, 256.0, 1.0));
    addInput(createInput<PJ301MPort>(Vec(100, 30), module, ModuleNews::IN_WRAP));
    addParam(createParam<TinyKnob>(Vec(105  , 60), module, ModuleNews::PARAM_WRAP, -31.0, 32.0, 0.0));

    for (int y = 0; y < GHEIGHT; ++y)
        for (int x = 0; x < GWIDTH; ++x) {
            int i = x + y * GWIDTH;
            addChild(createLight<CellLight<GreenLight>>(Vec(7 + x * 30, 100 + y * 30), module, ModuleNews::LIGHT_GRID + i));
            addOutput(createOutput<PJ301MPort>(Vec(7 + x * 30 + 2, 100 + y * 30 + 2), module, ModuleNews::OUT_CELL + i));
        }

    const float bottom_row = 345;
    addParam(createParam<CKSS>(Vec(5        , bottom_row), module, ModuleNews::PARAM_UNI_BI, 0.0, 1.0, 1.0));
    addParam(createParam<CKSS>(Vec(25       , bottom_row), module, ModuleNews::PARAM_MODE, 0.0, 1.0, 1.0));
    addParam(createParam<CKSS>(Vec(45       , bottom_row), module, ModuleNews::PARAM_GATEMODE, 0.0, 1.0, 1.0));
    addParam(createParam<CKSS>(Vec(65       , bottom_row), module, ModuleNews::PARAM_ROUND, 0.0, 1.0, 1.0));
    addParam(createParam<CKSS>(Vec(85       , bottom_row), module, ModuleNews::PARAM_CLAMP, 0.0, 1.0, 1.0));
    addParam(createParam<TinyKnob>(Vec(110  , bottom_row), module, ModuleNews::PARAM_SMOOTH, 0.0, 1.0, 0.0));
}

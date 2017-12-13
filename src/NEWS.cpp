#include "dsp/digital.hpp"
#include "math.hpp"
#include "qwelk.hpp"


#define GWIDTH  4
#define GHEIGHT 8


typedef unsigned char byte;


struct ModuleNews : Module {
    enum ParamIds {
        PARAM_MODE,
        PARAM_GATEMODE,
        PARAM_WIDTH,
        NUM_PARAMS
    };
    enum InputIds {
        IN_NEWS,
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

    byte grid[GWIDTH * GHEIGHT] {};    
    
    ModuleNews() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

void ModuleNews::step()
{
    bool    mode        = params[PARAM_MODE].value > 0.0;
    bool    gatemode    = params[PARAM_GATEMODE].value > 0.0;
    float   width       = params[PARAM_WIDTH].value;
    float   news        = width * inputs[IN_NEWS].value;

    unsigned key = (unsigned)(*((int *)&news));

    // reset grid
    for (int i = 0; i < GWIDTH * GHEIGHT; ++i)
        grid[i] = 0;

    // extract N-E-W-S info
    int up = (key >> 24) & 0xFF;
    int rt = (key >> 16) & 0xFF;
    int dn = (key >>  8) & 0xFF;
    int lt = (key      ) & 0xFF;

    // read the N-E-W-S
    int cy = GHEIGHT / 2, cx = GWIDTH / 2;

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

    byte intensity = 4;
    
    // light up cells and output
    for (int y = 0; y < GHEIGHT; ++y)
        for (int x = 0; x < GWIDTH; ++x) {
            int i = x + y * GWIDTH;
            byte r  = grid[i] * intensity - 1;
            float v = gatemode
                      ? (grid[i] ? 1 : 0)
                      : ((byte)r / 255.0);
            float l = v * 0.9;
            lights[LIGHT_GRID + i].setBrightness(l);
            outputs[OUT_CELL + i].value = 5.0 * v;
        }

}
#define LIGHT_SIZE 10
struct RoundTinyKnob : RoundBlackKnob {
	RoundTinyKnob()
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
    
    addInput(createInput<PJ301MPort>(Vec(10, 30), module, ModuleNews::IN_NEWS));
    addParam(createParam<RoundTinyKnob>(Vec(45, 30), module, ModuleNews::PARAM_WIDTH, 1e-6, 10000.0, 1.0));
    addParam(createParam<CKSS>(Vec(70, 30), module, ModuleNews::PARAM_MODE, 0.0, 1.0, 1.0));
    addParam(createParam<CKSS>(Vec(90, 30), module, ModuleNews::PARAM_GATEMODE, 0.0, 1.0, 1.0));

    for (int y = 0; y < GHEIGHT; ++y)
        for (int x = 0; x < GWIDTH; ++x) {
            int i = x + y * GWIDTH;
            addChild(createLight<CellLight<GreenLight>>(Vec(7 + x * 30, 60 + y * 30), module, ModuleNews::LIGHT_GRID + i));
            addOutput(createOutput<PJ301MPort>(Vec(7 + x * 30 + 2, 60 + y * 30 + 2), module, ModuleNews::OUT_CELL + i));
        }
}

#include "dsp/digital.hpp"
#include "math.hpp"
#include "qwelk.hpp"


#define GSIZE 8


typedef unsigned char byte;


struct ModuleNews : Module {
    enum ParamIds {
        PARAM_WIDTH,
        NUM_PARAMS
    };
    enum InputIds {
        IN_NEWS,
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightIds {
        LIGHT_GRID,
        NUM_LIGHTS = LIGHT_GRID + GSIZE * GSIZE
    };

    byte grid[GSIZE * GSIZE] {};    
    
    ModuleNews() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

void ModuleNews::step()
{
    float width = params[PARAM_WIDTH].value;
    float news = width * inputs[IN_NEWS].value;
    unsigned key = (unsigned)(*((int *)&news));

    for (int i = 0; i < GSIZE * GSIZE; ++i)
        grid[i] = 0;

    int up = (key >> 24) & 0xFF;
    int rt = (key >> 16) & 0xFF;
    int dn = (key >>  8) & 0xFF;
    int lt = (key      ) & 0xFF;

    int cy = GSIZE / 2, cx = GSIZE / 2;
    for (int i = 0; i < up; ++i) {
        cy = (cy - 1) >= 0 ? cy - 1 : GSIZE - 1;
        grid[cx + cy * GSIZE] ^= 1;
    }
    for (int i = 0; i < rt; ++i) {
        cx = (cx + 1) < GSIZE ? cx + 1 : 0;
        grid[cx + cy * GSIZE] ^= 1;
    }
    for (int i = 0; i < dn; ++i) {
        cy = (cy + 1) < GSIZE ? cy + 1 : 0;
        grid[cx + cy * GSIZE] ^= 1;
    }
    for (int i = 0; i < lt; ++i) {
        cx = (cx - 1) >= 0 ? cx - 1 : GSIZE;
        grid[cx + cy * GSIZE] ^= 1;
    }
    
    // blink according to state
    for (int y = 0; y < GSIZE; ++y)
        for (int x = 0; x < GSIZE; ++x) {
            int i = x + y * GSIZE;
            lights[LIGHT_GRID + i].setBrightness(grid[i] ? 0.9 : 0.0);
        }
}

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
        this->box.size = mm2px(Vec(6, 6));
    }
};

WidgetNews::WidgetNews()
{
    ModuleNews *module = new ModuleNews();
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
    
    addInput(createInput<PJ301MPort>(Vec(15, 30), module, ModuleNews::IN_NEWS));
    addParam(createParam<RoundTinyKnob> (Vec(50, 30), module, ModuleNews::PARAM_WIDTH, 1e-6, 10000.0, 1.0));
    
    for (int y = 0; y < GSIZE; ++y)
        for (int x = 0; x < GSIZE; ++x) {
            int i = x + y * GSIZE;
            addChild(createLight<CellLight<GreenLight>>(Vec(15 + x * 25, 60 + y * 25), module, ModuleNews::LIGHT_GRID + i));
        }
}

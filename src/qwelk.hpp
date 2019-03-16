#include "rack.hpp"

using namespace rack;

extern Plugin *plugin;

extern Model *modelAutomaton;
extern Model *modelByte;
extern Model *modelChaos;
extern Model *modelColumn;
extern Model *modelGate;
extern Model *modelOr;
extern Model *modelNot;
extern Model *modelXor;
extern Model *modelMix;
extern Model *modelNews;
extern Model *modelScaler;
extern Model *modelWrap;
extern Model *modelXFade;
extern Model *modelIndra;

struct TinyKnob : RoundKnob {
    TinyKnob() {
        setSVG(SVG::load(assetPlugin(plugin, "res/TinyKnob.svg")));
    }
};

struct SmallKnob : RoundKnob {
    SmallKnob() {
        setSVG(SVG::load(assetPlugin(plugin, "res/SmallKnob.svg")));
    }
};
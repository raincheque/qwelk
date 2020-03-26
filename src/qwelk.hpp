#include "rack.hpp"

using namespace rack;

extern Plugin *pluginInstance;

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
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyKnob.svg")));
    }
};

struct SmallKnob : RoundKnob {
    SmallKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SmallKnob.svg")));
    }
};

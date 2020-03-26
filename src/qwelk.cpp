#include "qwelk.hpp"

Plugin *pluginInstance;

void init(rack::Plugin *p) {
    pluginInstance = p;
	
    p->addModel(modelAutomaton);
    p->addModel(modelByte);
    p->addModel(modelChaos);
    p->addModel(modelColumn);
    p->addModel(modelGate);
    p->addModel(modelOr);
    p->addModel(modelNot);
    p->addModel(modelXor);
    p->addModel(modelMix);
    p->addModel(modelNews);
    p->addModel(modelScaler);
    p->addModel(modelWrap);
    p->addModel(modelXFade);
    p->addModel(modelIndra);
}

#include "qwelk.hpp"

Plugin *plugin;

void init(rack::Plugin *p) {
    plugin = p;
	
	p->slug = TOSTRING(SLUG);
	p->version = TOSTRING(VERSION);

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

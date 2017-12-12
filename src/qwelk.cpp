#include "qwelk.hpp"

Plugin *plugin;

void init(rack::Plugin *p) {
    plugin = p;
	// This is the unique identifier for your plugin
	p->slug = "Qwelk";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif
	p->website = "https://github.com/raincheque/qwelk";
	// p->manual = "https://github.com/VCVRack/Tutorial/blob/master/README.md";

	// For each module, specify the ModuleWidget subclass, manufacturer slug (for saving in patches), manufacturer human-readable name, module slug, and module name
	p->addModel(createModel<WidgetAutomaton>("Qwelk", "Automaton", "Automaton", SEQUENCER_TAG));
    p->addModel(createModel<WidgetChaos>("Qwelk", "Chaos", "Chaos", SEQUENCER_TAG));
    p->addModel(createModel<WidgetByte>("Qwelk", "Byte", "Byte", UTILITY_TAG));
    p->addModel(createModel<WidgetScaler>("Qwelk", "Scaler", "Scaler", UTILITY_TAG));
    p->addModel(createModel<WidgetXFade>("Qwelk", "XFade", "XFade", UTILITY_TAG));
    p->addModel(createModel<WidgetOr>("Qwelk", "OR", "OR", UTILITY_TAG));
    p->addModel(createModel<WidgetNot>("Qwelk", "NOT", "NOT", UTILITY_TAG));
    p->addModel(createModel<WidgetXor>("Qwelk", "XOR", "XOR", UTILITY_TAG));
    p->addModel(createModel<WidgetNews>("Qwelk", "NEWS", "NEWS", SEQUENCER_TAG));

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}

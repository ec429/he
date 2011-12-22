#include "info_choice.h"
#include <stdbool.h>
#include "bits.h"

#if INFO_MIDI
int render_midi(unsigned int addr, string bytes, bool draw, int maxw);

int minf; // offset of midi in the infos
unsigned int meat; // for cursor movement to use (right-cursor moves meat bytes instead of just 1)
#endif

#include "info_choice.h"
#if INFO_Z80_DISASSEMBLER
#include "stdbool.h"
#include "bits.h"
int render_z80disasm(unsigned int addr, string bytes, bool draw, int maxw);

int zinf; // offset of the z80disasm in the infos
unsigned int zeat; // for cursor movement to use (right-cursor moves zeat bytes instead of just 1)
#endif

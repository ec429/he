#include "info_choice.h"
#if INFO_Z80_DISASSEMBLER
#include "stdbool.h"
#include "bits.h"

typedef struct
{
	int shift, x, y, p, q, z;
	int eat;
}
z80disasm;

int render_z80disasm(unsigned int addr, string bytes, bool draw, int maxw);
z80disasm z80disasm_opcode(unsigned int addr, string bytes, char *what);

int zinf; // offset of the z80disasm in the infos
unsigned int zeat; // for cursor movement to use (right-cursor moves zeat bytes instead of just 1)
#endif

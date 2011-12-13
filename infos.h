#include <stdbool.h>
#include "bits.h"

typedef struct
{
	const char *name;
	int minw;
	int (*render)(unsigned int addr, string bytes, bool draw, int maxw); // returns width used
}
info;

unsigned int ninfos;
info *infos;
bool *display;

int init_infos(void);
unsigned int countirows(unsigned int addr, string bytes, unsigned int cols);
void render_irows(unsigned int addr, string bytes, unsigned int cols, unsigned int basey);

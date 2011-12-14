#include "info_choice.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
	uint32_t code; // codepoint
	char *name; // Unicode Name
	bool isdup; // is this point's name field a duplicate?
}
uni_pt;

#if INFO_UNICODE_UTF8
uint32_t upts;
uni_pt *utbl;

int init_unicode(void);
int render_utf8(unsigned int addr, string bytes, bool draw, int maxw);

int uinf; // offset of the utf8 in the infos
unsigned int ueat; // for cursor movement to use (right-cursor moves ueat bytes instead of just 1)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "bits.h"
#include "z80.h"

int main(int argc, char *argv[])
{
	const char *file=NULL;
	for(int arg=1;arg<argc;arg++)
	{
		file=argv[arg];
	}
	FILE *f=fopen(file, "rb");
	if(!f)
	{
		fprintf(stderr, "z80disasm: Failed to open '%s': fopen: %s\n", file, strerror(errno));
		return(EXIT_FAILURE);
	}
	string fbuf;
	if(!(fbuf=sslurp(f)).buf)
	{
		fprintf(stderr, "z80disasm: Failed to read from file\n");
		return(EXIT_FAILURE);
	}
	fclose(f);
	unsigned int addr=0;
	char what[32]="";
	while(1)
	{
		z80disasm z=z80disasm_opcode(addr, fbuf, what);
		if(z.eat)
		{
			printf("\t%s", what);
			for(int i=strlen(what);i<24;i++) putchar(' ');
			printf("; %08x\n", addr);
			addr+=z.eat;
		}
		else
			break;
	}
	return(EXIT_SUCCESS);
}

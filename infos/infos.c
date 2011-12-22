#include <stdlib.h>
#include <curses.h>
#include "infos.h"
#include "info_choice.h"

#if INFO_DECIMAL_READS
int render_decimalreads(unsigned int addr, string bytes, bool draw, int maxw)
{
	if(bytes.i<=addr) return(0);
	int w=0;
	int pw[6]={7, 8, 10, 11, 11, 12};
	bool parts[6];
	for(int p=0;p<6;p++)
	{
		if(maxw-w>=pw[p])
		{
			parts[p]=true;
			w+=pw[p];
		}
	}
	if(draw)
	{
		unsigned char b=bytes.buf[addr];
		unsigned int s,as;
		if(lendian)
		{
			s=bytes.buf[addr]+((addr+1<bytes.i?bytes.buf[addr+1]:0)<<8);
			as=addr&1?(unsigned int)bytes.buf[addr-1]+(bytes.buf[addr]<<8):s;
		}
		else
		{
			s=(bytes.buf[addr]<<8)+(addr+1<bytes.i?bytes.buf[addr+1]:0);
			as=addr&1?((unsigned int)bytes.buf[addr-1]<<8)+bytes.buf[addr]:s;
		}
		if(parts[0]) printw("U8:%03hhu ", b);
		if(parts[1]) printw("S8:%0+4hhd ", b);
		if(parts[2]) printw("U16:%05hu ", s);
		if(parts[3]) printw("S16:%0+6hd ", s);
		if(parts[4]) printw("AU16:%05hu ", as);
		if(parts[5]) printw("AS16:%0+6hd ", as);
	}
	return(w);
}
#endif

#if INFO_BINARY
int render_binary(unsigned int addr, string bytes, bool draw, int maxw)
{
	if(bytes.i<=addr) return(0);
	if(maxw<13) return(0);
	if(draw)
	{
		char bin[9];
		bin[8]=0;
		unsigned char b=bytes.buf[addr];
		for(int i=0;i<8;i++)
		{
			bin[i]=(b&0x80)?'1':'0';
			b<<=1;
		}
		printw("BIN:%s ", bin);
	}
	return(13);
}
#endif

#if INFO_Z80_DISASSEMBLER
#include "z80.h"
#endif

#if INFO_UNICODE_UTF8
#include "utf8.h"
#endif

#if INFO_MIDI
#include "midi.h"
#endif

int add_info(const char *name, int minw, int render(unsigned int, string, bool, int), int);

int init_infos(void)
{
	ninfos=0;
	infos=NULL;
	int e=0;
	#if INFO_DECIMAL_READS
	if((e=add_info("Decimal Reads", 7, render_decimalreads, '#'))) { free(infos); return(e); }
	#endif
	#if INFO_BINARY
	if((e=add_info("Binary Byte", 13, render_binary, '!'))) { free(infos); return(e); }
	#endif
	#if INFO_Z80_DISASSEMBLER
	zinf=ninfos;
	if((e=add_info("Z80 Disassembler", 23, render_z80disasm, 4))) { free(infos); return(e); }
	#endif
	#if INFO_UNICODE_UTF8
	uinf=ninfos;
	if((e=add_info("Unicode UTF-8", 13, render_utf8, '"'))) { free(infos); return(e); }
	#endif
	#if INFO_MIDI
	minf=ninfos;
	if((e=add_info("MIDI", 40, render_midi, 'M'))) { free(infos); return(e); }
	#endif
	display=malloc(ninfos*sizeof(bool));
	if(!display) { free(infos); return(-1); }
	for(unsigned int i=0;i<ninfos;i++) display[i]=false;
	return(0);
}

int add_info(const char *name, int minw, int render(unsigned int, string, bool, int), int key)
{
	unsigned int n=ninfos++;
	info *ni=realloc(infos, ninfos*sizeof(info));
	if(!ni)
	{
		ninfos=n;
		return(-1);
	}
	(infos=ni)[n]=(info){.name=name, .minw=minw, .render=render, .key=key};
	return(0);
}

unsigned int countirows(unsigned int addr, string bytes, unsigned int cols)
{
	unsigned int uw[32], rows=0;
	for(unsigned int i=0;i<ninfos;i++)
	{
		if(display[i])
		{
			unsigned int r;
			for(r=0;r<rows;r++)
			{
				int w=cols-uw[r];
				if(w>=infos[i].minw)
				{
					int u=infos[i].render(addr, bytes, false, w);
					if(u<=w)
					{
						uw[r]+=u;
						break;
					}
				}
			}
			if(r==rows)
			{
				uw[r]=0;
				int w=cols-uw[r];
				if(w>=infos[i].minw)
				{
					int u=infos[i].render(addr, bytes, false, w);
					if(u<=w)
					{
						uw[r]=u;
						rows++;
					}
				}
			}
		}
	}
	return(rows);
}

void render_irows(unsigned int addr, string bytes, unsigned int cols, unsigned int basey)
{
	unsigned int uw[32], rows=0;
	for(unsigned int i=0;i<ninfos;i++)
	{
		if(display[i])
		{
			unsigned int r;
			for(r=0;r<rows;r++)
			{
				int w=cols-uw[r];
				if(w>=infos[i].minw)
				{
					int u=infos[i].render(addr, bytes, false, w);
					if(u<=w)
					{
						move(basey+r, uw[r]);
						infos[i].render(addr, bytes, true, w);
						clrtoeol();
						uw[r]+=u;
						break;
					}
				}
			}
			if(r==rows)
			{
				uw[r]=0;
				int w=cols-uw[r];
				if(w>=infos[i].minw)
				{
					int u=infos[i].render(addr, bytes, false, w);
					if(u<=w)
					{
						move(basey+r, uw[r]);
						infos[i].render(addr, bytes, true, w);
						clrtoeol();
						uw[r]=u;
						rows++;
					}
				}
			}
		}
	}
}

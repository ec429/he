#include "midi.h"
#include <curses.h>

#if INFO_MIDI
const char *command[8]={"NoteOff","NoteOn","AfterTouch","ControlChange","ProgramChange","Expression","PitchWheel","System"};
const char *notename[12]={"C","C#","D","Eb","E","F","F#","G","G#","A","Bb","B"};

int render_midi(unsigned int addr, string bytes, bool draw, int maxw)
{
	unsigned int eat=0;
	unsigned int delt=0;
	unsigned char c=0xff;
	while(addr+eat<bytes.i)
	{
		c=bytes.buf[addr+eat++];
		delt=(delt<<7)|(c&0x7f);
		if(!(c&0x80)) break;
	}
	if(c&0x80)
		return(0);
	char d[16];
	int len;
	snprintf(d, 16, "%u%n", delt, &len);
	if(addr+eat<bytes.i)
	{
		c=bytes.buf[addr+eat++];
		if(c&0x80)
		{
			unsigned char cmd=(c>>4)&7, chan=c&0xf;
			if(cmd==7)
			{
				const char *cmdname=command[cmd];
				size_t clen=strlen(cmdname);
				if(len+clen+10>(unsigned)maxw)
					return(0);
				if(draw)
					printw("MIDI: +%st %s ", d, cmdname);
				return(len+clen+10);
			}
			else
			{
				const char *cmdname=command[cmd];
				size_t clen=strlen(cmdname);
				char params[80];
				switch(cmd)
				{
					case 0: // NoteOff	nn vv
					case 1: // NoteOn	nn vv
					case 2: // AfterTch	nn vv
					{
						unsigned char nn=0xff, vv=0xff;
						if(addr+eat<bytes.i)
							nn=bytes.buf[addr+eat];
						eat++;
						if(addr+eat<bytes.i)
							vv=bytes.buf[addr+eat];
						eat++;
						if(nn&0x80)
						{
							eat-=2;
							strcpy(params, "nn vv ");
							break;
						}
						if(vv&0x80)
						{
							eat--;
							snprintf(params, 80, "%s%u vv ", notename[nn%12], nn/12);
							break;
						}
						snprintf(params, 80, "%s%u %u ", notename[nn%12], nn/12, vv);
					}
					break;
					case 3: // ControlChange cc vv
					{
						unsigned char cc=0xff, vv=0xff;
						if(addr+eat<bytes.i)
							cc=bytes.buf[addr+eat];
						eat++;
						if(addr+eat<bytes.i)
							vv=bytes.buf[addr+eat];
						eat++;
						if(cc&0x80)
						{
							eat-=2;
							strcpy(params, "cc vv ");
							break;
						}
						if(cc&0x80)
						{
							eat--;
							snprintf(params, 80, "%02x vv ", cc);
							break;
						}
						snprintf(params, 80, "%02x %u ", cc, vv); // TODO RPN controller names
					}
					break;
					case 4: // ProgramChange pp
					{
						unsigned char pp=0xff;
						if(addr+eat<bytes.i)
							pp=bytes.buf[addr+eat];
						eat++;
						if(pp&0x80)
						{
							eat--;
							strcpy(params, "pp ");
							break;
						}
						snprintf(params, 80, "%u ", pp); // TODO General MIDI program names
					}
					break;
					case 5: // Expression vv (channel AfterTouch)
					{
						unsigned char vv=0xff;
						if(addr+eat<bytes.i)
							vv=bytes.buf[addr+eat];
						eat++;
						if(vv&0x80)
						{
							eat--;
							strcpy(params, "vv ");
							break;
						}
						snprintf(params, 80, "%u ", vv);
					}
					break;
					case 6: // Pitch Wheel bb tt
					{
						unsigned char bb=0xff, tt=0xff;
						signed int pw=0;
						if(addr+eat<bytes.i)
							bb=bytes.buf[addr+eat];
						eat++;
						if(addr+eat<bytes.i)
							tt=bytes.buf[addr+eat];
						eat++;
						if(bb&0x80)
						{
							eat-=2;
							strcpy(params, "bb tt ");
							break;
						}
						if(tt&0x80)
						{
							eat--;
							snprintf(params, 80, "%02x tt ", bb);
							break;
						}
						pw=((tt<<7)|bb)-0x2000;
						snprintf(params, 80, "%+d ", pw);
					}
					break;
					default:
						params[0]=0;
					break;
				}
				size_t plen=strlen(params);
				if(len+clen+plen+14>(unsigned)maxw)
					return(0);
				if(draw)
					printw("MIDI: +%st [%u] %s %s", d, chan, cmdname, params);
				return(len+clen+plen+14);
			}
			return(0);
		}
	}
	if(len+9>maxw)
		return(0);
	if(draw)
		printw("MIDI: +%st ", d);
	return(len+9);
}
#endif

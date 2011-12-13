#include <curses.h>
#include "z80.h"

#if INFO_Z80_DISASSEMBLER

const char *tbl_r[8]={"B","C","D","E","H","L","(HL)","A"};
const char *tbl_rp[4]={"BC","DE","HL","SP"};
const char *tbl_rp2[4]={"BC","DE","HL","AF"};
const char *tbl_cc[8]={"NZ","Z","NC","C","PO","PE","P","M"};
const char *tbl_alu[8]={"ADD A,","ADC A,","SUB ","SBC A,","AND ","XOR ","OR ","CP "};
const char *tbl_rot[8]={"RLC","RRC","RL","RR","SLA","SRA","SLL","SRL"};
const char *tbl_im[4]={"0","1","1","2"};
const char *tbl_bli_a[4]={"I","D","IR","DR"};
const char *tbl_bli_b[4]={"LD","CP","IN","OT"};

int render_z80disasm(unsigned int addr, string bytes, bool draw, int maxw)
{
	if(bytes.i<=addr) return(0);
	int more=bytes.i-addr;
	unsigned char b=bytes.buf[addr];
	unsigned char x=b>>6, y=(b>>3)&7, z=b&7, p=y>>1, q=y&1;
	char what[32]="";
	switch(x)
	{
		case 0:
			switch(z)
			{
				case 0:
					switch(y)
					{
						case 0:
							strcpy(what, "NOP");
						break;
						case 1:
							strcpy(what, "EX AF,AF'");
						break;
						case 2:
							if(more>1)
								sprintf(what, "DJNZ %d", bytes.buf[addr+1]);
							else
								strcpy(what, "DJNZ d");
						break;
						case 3:
							if(more>1)
								sprintf(what, "JR %d", bytes.buf[addr+1]);
							else
								strcpy(what, "JR d");
						break;
						default:
							if(more>1)
								sprintf(what, "JR %s,%d", tbl_cc[y-4], bytes.buf[addr+1]);
							else
								sprintf(what, "JR %s,d", tbl_cc[y-4]);
						break;
					}
				break;
				default:
					strcpy(what, "Error z");
				break;
			}
		break;
		default:
			strcpy(what, "Error x");
		break;
	}
	int w=5+strlen(what);
	if(w>maxw) return(0);
	if(draw)
	{
		if(w+18<=maxw)
			printw("X:%hhu Y:%hhu p%hhu q%hhu Z:%hhu ", x, y, p, q, z);
		printw("Z80:%s ", what);
	}
	if(w+18<=maxw) w+=18;
	return(w);
}
#endif

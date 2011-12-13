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
	zeat=1;
	if(bytes.i<=addr) return(0);
	int more=bytes.i-addr;
	unsigned char b=bytes.buf[addr];
	int shift=0, prefs=0;
	while((b==0xDD)||(b==0xED)||(b==0xFD))
	{
		shift=(b-0xCD)>>4;
		more--;
		prefs++;
		if(!more)
		{
			shift=0;
			break;
		}
		b=bytes.buf[addr+prefs];
	}
	if((b==0xCB)&&(shift!=2))
	{
		more--;
		prefs++;
		if(more)
		{
			shift|=4;
			b=bytes.buf[addr+prefs+(shift&3?1:0)];
		}
	}
	unsigned char x=b>>6, y=(b>>3)&7, z=b&7, p=y>>1, q=y&1;
	int eat=1;
	char what[32]="";
	int ixy=shift&3;
	if(ixy==3) ixy=2;
	const char *ixify=((y==4)||(y==5))?((const char *[3]){"","IX","IY"}[ixy]):"";
	const char *ixifz=((z==4)||(z==5))?((const char *[3]){"","IX","IY"}[ixy]):"";
	const char *hixy=(const char *[3]){"HL","IX","IY"}[ixy];
	if(shift&4)
	{
		if(ixy)
		{
			switch(x)
			{
				case 0:
					eat=2;
					if(more>1)
						sprintf(what, "%s%s%s%s (%s%+hhd)", (z==6)?"":"LD ", (z==6)?"":tbl_r[z], (z==6)?"":",", tbl_rot[y], hixy, bytes.buf[addr+prefs]);
					else
						sprintf(what, "%s%s%s%s (%s+d)", (z==6)?"":"LD ", (z==6)?"":tbl_r[z], (z==6)?"":",", tbl_rot[y], hixy);
				break;
				case 1:
					if(z==6)
					{
						eat=2;
						if(more>1)
							sprintf(what, "BIT %hhd, (%s%+hhd)", y, hixy, bytes.buf[addr+prefs]);
						else
							sprintf(what, "BIT %hhd, (%s+d)", y, hixy);
					}
					else
						sprintf(what, "BIT %hhd,%s%s", y, ixifz, tbl_r[z]);
				break;
				case 2:
					eat=2;
					if(more>1)
						sprintf(what, "%s%s%sRES %hhd,(%s%+hhd)", (z==6)?"":"LD ", (z==6)?"":tbl_r[z], (z==6)?"":",", y, hixy, bytes.buf[addr+prefs]);
					else
						sprintf(what, "%s%s%sRES %hhd,(%s+d)", (z==6)?"":"LD ", (z==6)?"":tbl_r[z], (z==6)?"":",", y, hixy);
				break;
				case 3:
					eat=2;
					if(more>1)
						sprintf(what, "%s%s%sSET %hhd,(%s%+hhd)", (z==6)?"":"LD ", (z==6)?"":tbl_r[z], (z==6)?"":",", y, hixy, bytes.buf[addr+prefs]);
					else
						sprintf(what, "%s%s%sSET %hhd,(%s+d)", (z==6)?"":"LD ", (z==6)?"":tbl_r[z], (z==6)?"":",", y, hixy);
				break;
			}
		}
		else
		{
			switch(x)
			{
				case 0:
					sprintf(what, "%s %s", tbl_rot[y], tbl_r[z]);
				break;
				case 1:
					sprintf(what, "BIT %hhd,%s", y, tbl_r[z]);
				break;
				case 2:
					sprintf(what, "RES %hhd,%s", y, tbl_r[z]);
				break;
				case 3:
					sprintf(what, "SET %hhd,%s", y, tbl_r[z]);
				break;
				default:
					strcpy(what, "Error x");
				break;
			}
		}
	}
	else if(shift&2)
	{
		strcpy(what, "ED not done!");
	}
	else
	{
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
								eat=2;
								if(more>1)
									sprintf(what, "DJNZ %hd", bytes.buf[addr+prefs+1]);
								else
									strcpy(what, "DJNZ d");
							break;
							case 3:
								eat=2;
								if(more>1)
									sprintf(what, "JR %hd", bytes.buf[addr+prefs+1]);
								else
									strcpy(what, "JR d");
							break;
							default:
								eat=2;
								if(more>1)
									sprintf(what, "JR %s,%hd", tbl_cc[y-4], bytes.buf[addr+prefs+1]);
								else
									sprintf(what, "JR %s,d", tbl_cc[y-4]);
							break;
						}
					break;
					case 1:
						if(!q)
						{
							eat=3;
							if(more>2)
								sprintf(what, "LD %s,%hu", (p==2)?hixy:tbl_rp[p], bytes.buf[addr+prefs+1]+(bytes.buf[addr+prefs+2]<<8));
							else
								sprintf(what, "LD %s,nn", (p==2)?hixy:tbl_rp[p]);
						}
						else
						{
							if(p==2)
								sprintf(what, "ADD %s,%s", hixy, hixy);
							else
								sprintf(what, "ADD %s,%s", hixy, tbl_rp[p]);
						}
					break;
					case 2:
					{
						const char *a, *b;
						char nn[7];
						if(more>2)
							sprintf(nn, "(%04x)", bytes.buf[addr+prefs+1]+(bytes.buf[addr+prefs+2]<<8));
						else
							strcpy(nn, "(nn)");
						switch(p)
						{
							case 0:
								a="(BC)";
								b="A";
							break;
							case 1:
								a="(DE)";
								b="A";
							break;
							case 2:
								eat=3;
								a=nn;
								b=hixy;
							break;
							case 3:
								eat=3;
								a=nn;
								b="A";
							break;
						}
						sprintf(what, "LD %s,%s", q?b:a, q?a:b);
					}
					break;
					case 3:
						if(p==2)
							sprintf(what, "%s %s", q?"DEC":"INC", hixy);
						else
							sprintf(what, "%s %s", q?"DEC":"INC", tbl_rp[p]);
					break;
					case 4:
						if(ixy&&(y==6))
						{
							eat=2;
							if(more>1)
								sprintf(what, "INC (%s%+hhd)", hixy, bytes.buf[addr+prefs+1]);
							else
								sprintf(what, "INC (%s+d)", hixy);
						}
						else
							sprintf(what, "INC %s%s", ixify, tbl_r[y]);
					break;
					case 5:
						if(ixy&&(y==6))
						{
							eat=2;
							if(more>1)
								sprintf(what, "DEC (%s%+hhd)", hixy, bytes.buf[addr+prefs+1]);
							else
								sprintf(what, "DEC (%s+d)", hixy);
						}
						else
							sprintf(what, "DEC %s%s", ixify, tbl_r[y]);
					break;
					case 6:
						if(ixy&&(y==6))
						{
							eat=3;
							if(more>2)
								sprintf(what, "LD (%s%+hhd),%hhu", hixy, bytes.buf[addr+prefs+1], bytes.buf[addr+prefs+2]);
							else
								sprintf(what, "LD (%s+d),n", hixy);
						}
						else
						{
							eat=2;
							if(more>1)
								sprintf(what, "LD %s%s,%hhu", ixify, tbl_r[y], bytes.buf[addr+prefs+1]);
							else
								sprintf(what, "LD %s%s,n", ixify, tbl_r[y]);
						}
					break;
					case 7:
						strcpy(what, (const char *[8]){"RLCA","RRCA","RLA","RRA","DAA","CPL","SCF","CCF"}[y]);
					break;
					default:
						strcpy(what, "Error z");
					break;
				}
			break;
			case 1:
				if((y==6)&&(z==6))
					strcpy(what, "HALT");
				else if(ixy&&(y==6))
				{
					eat=2;
					if(more>1)
						sprintf(what, "LD (%s%+hhd),%s", hixy, bytes.buf[addr+prefs+1], tbl_r[z]);
					else
						sprintf(what, "LD (%s+d),%s", hixy, tbl_r[z]);
				}
				else if(ixy&&(z==6))
				{
					eat=2;
					if(more>1)
						sprintf(what, "LD %s,(%s%+hhd)", tbl_r[y], hixy, bytes.buf[addr+prefs+1]);
					else
						sprintf(what, "LD %s,(%s+d)", tbl_r[y], hixy);
				}
				else
					sprintf(what, "LD %s%s,%s%s", ixify, tbl_r[y], ixifz, tbl_r[z]);
			break;
			case 2:
				if(ixy&&(z==6))
				{
					eat=2;
					if(more>1)
						sprintf(what, "%s (%s%+hhd)", tbl_alu[y], hixy, bytes.buf[addr+prefs+1]);
					else
						sprintf(what, "%s (%s+d)", tbl_alu[y], hixy);
				}
				else
					sprintf(what, "%s%s%s", tbl_alu[y], ixifz, tbl_r[z]);
			break;
			case 3:
				switch(z)
				{
					case 0:
						sprintf(what, "RET %s", tbl_cc[y]);
					break;
					case 1:
						if(!q)
							if(p==2)
								sprintf(what, "POP %s", hixy);
							else
								sprintf(what, "POP %s", tbl_rp2[p]);
						else
							sprintf(what, (const char *[4]){"RET","EXX","JP %s","LD SP,%s"}[p], hixy);
					break;
					case 2:
						eat=3;
						if(more>2)
							sprintf(what, "JP %s,%hu", tbl_cc[y], bytes.buf[addr+prefs+1]+(bytes.buf[addr+prefs+2]<<8));
						else
							sprintf(what, "JP %s,nn", tbl_cc[y]);
					break;
					case 3:
						switch(y)
						{
							case 0:
								eat=3;
								if(more>2)
									sprintf(what, "JP %hu", bytes.buf[addr+prefs+1]+(bytes.buf[addr+prefs+2]<<8));
								else
									strcpy(what, "JP nn");
							break;
							case 1:
								strcpy(what, "Error CB");
							break;
							case 2:
								eat=2;
								if(more>1)
									sprintf(what, "OUT (%hhu),A", bytes.buf[addr+prefs+1]);
								else
									strcpy(what, "OUT (n),A");
							break;
							case 3:
								eat=2;
								if(more>1)
									sprintf(what, "IN A,(%hhu)", bytes.buf[addr+prefs+1]);
								else
									strcpy(what, "IN A,(n)");
							break;
							default:
								sprintf(what, (const char *[4]){"EX (SP),%s","EX DE,HL","DI","EI"}[y-4], hixy); // EX DE,HL is *unaffected* by DD/FD prefixes!
							break;
						}
					break;
					case 4:
						eat=3;
						if(more>2)
							sprintf(what, "CALL %s,%hu", tbl_cc[y], bytes.buf[addr+prefs+1]+(bytes.buf[addr+prefs+2]<<8));
						else
							sprintf(what, "CALL %s,nn", tbl_cc[y]);
					break;
					case 5:
						if(!q)
							if(p==2)
								sprintf(what, "PUSH %s", hixy);
							else
								sprintf(what, "PUSH %s", tbl_rp2[p]);
						else switch(p)
						{
							case 0:
								eat=3;
								if(more>2)
									sprintf(what, "CALL %hu", bytes.buf[addr+prefs+1]+(bytes.buf[addr+prefs+2]<<8));
								else
									strcpy(what, "CALL nn");
							break;
							case 1:
								strcpy(what, "Error DD");
							break;
							case 2:
								strcpy(what, "Error ED");
							break;
							case 3:
								strcpy(what, "Error FD");
							break;
							default:
								strcpy(what, "Error p");
							break;
						}
					break;
					case 6:
						eat=2;
						if(more>1)
							sprintf(what, "%s%hhu", tbl_alu[y], bytes.buf[addr+prefs+1]);
						else
							sprintf(what, "%sn", tbl_alu[y]);
					break;
					case 7:
						sprintf(what, "RST %hhu", y<<3);
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
	}
	eat+=prefs;
	int w=5+strlen(what);
	if(w>maxw) return(0);
	bool xyz=(w+22<=maxw);
	if(xyz) w+=22;
	bool sheat=(w+6<=maxw);
	if(sheat) w+=6;
	if(draw)
	{
		if(xyz)
			printw("S:%hhu X:%hhu Y:%hhu p%hhu q%hhu Z:%hhu ", shift, x, y, p, q, z);
		if(sheat)
			printw("Len:%hhu ", eat);
		printw("Z80:%s ", what);
	}
	zeat=eat;
	return(w);
}
#endif

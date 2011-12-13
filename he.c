/*
	he - a hex editor
	Copyright (C) 2011 Edward Cree

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>
#include <locale.h>
#include <errno.h>

#include "bits.h"
#include "keynames.h"
#include "infos.h"
#include "info_choice.h"

#if INFO_Z80_DISASSEMBLER
#include "z80.h"
#endif

#define VERSION	"0.1.0"

int initialise_curses(void);
void status(const char *st);
void curs_status(unsigned int y, unsigned int x, unsigned int hcols, unsigned int scroll);
void draw_title(const char *file, string fbuf, bool unsaved, size_t oldsize);

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	char *file=NULL;
	for(int arg=1;arg<argc;arg++)
	{
		file=argv[arg];
	}
	string fbuf=null_string();
	if(file)
	{
		FILE *f=fopen(file, "rb");
		if(!f)
		{
			fprintf(stderr, "he: file '%s' could not be opened: %s\n", file, strerror(errno));
			return(EXIT_FAILURE);
		}
		if(!(fbuf=sslurp(f)).buf) file=NULL;
		fclose(f);
	}
	if(initialise_curses())
	{
		fprintf(stderr, "he: curses was not set up correctly\n");
		return(EXIT_FAILURE);
	}
	if(init_infos())
	{
		fprintf(stderr, "he: failed to init_infos\n");
		return(EXIT_FAILURE);
	}
	bool unsaved=false;
	size_t oldsize=fbuf.i;
	unsigned int rows, cols;
	getmaxyx(stdscr, rows, cols);
	draw_title(file, fbuf, unsaved, oldsize);
	unsigned int hcols=(cols-13)/4;
	status("Ready");
	unsigned int scroll=0, cursy=0, cursx=0;
	bool left=true;
	unsigned int f1cycle=0;
	lendian=false;
	int half=0;
	int errupt=0;
	while(!errupt)
	{
		unsigned int irows=1+countirows((cursy+scroll)*hcols+cursx, fbuf, cols);
		unsigned int scrolljump=(rows-irows-2)/2;
		render_irows((cursy+scroll)*hcols+cursx, fbuf, cols, rows-irows);
		unsigned int y, x;
		for(y=2;y<rows-irows;y++)
		{
			mvprintw(y, 0, "0x%08x ", (y+scroll-2)*hcols);
			for(x=0;x<hcols;x++)
			{
				unsigned int addr=(y+scroll-2)*hcols+x;
				if(addr<fbuf.i)
				{
					unsigned char c=fbuf.buf[addr];
					if((y==cursy+2)&&(x==cursx))
					{
						if(half)
						{
							attron(A_BOLD);
							mvprintw(y, (x*3)+12, "%02x", c);
							attron(A_STANDOUT);
							mvprintw(y, (x*3)+13, "%01x", c&0xf);
						}
						else
						{
							attron(left?A_STANDOUT|A_BOLD:A_BOLD);
							mvprintw(y, (x*3)+12, "%02x", c);
						}
					}
					else
						mvprintw(y, (x*3)+12, "%02x", c);
					attroff(A_STANDOUT|A_BOLD);
					addch(' ');
					if(c&0x80)
						attron(A_REVERSE);
					c&=0x7f;
					bool pr=(c>=0x20)&&(c<0x7F);
					if((y==cursy+2)&&(x==cursx))
						attron(left?A_BOLD:A_STANDOUT|A_BOLD);
					mvaddch(y, x+(hcols*3)+13, pr?c:'.');
					attroff(A_REVERSE|A_STANDOUT|A_BOLD);
				}
				else
				{
					if((y==cursy+2)&&(x==cursx))
						attron(A_STANDOUT);
					if(left)
						mvprintw(y, (x*3)+12, "  ");
					else
						mvaddch(y, x+(hcols*3)+13, ' ');
					attroff(A_STANDOUT);
					if(!left)
						mvprintw(y, (x*3)+12, "  ");
					else
						mvaddch(y, x+(hcols*3)+13, ' ');
				}
			}
			if(x<hcols) break;
		}
		int key=getch();
		if(left&&isxdigit(key))
		{
			char h[2]={key, 0};
			unsigned int hv;
			if(sscanf(h, "%x", &hv)==1)
			{
				unsigned int addr=(scroll+cursy)*hcols+cursx;
				if(addr<fbuf.i)
				{
					unsigned char b=fbuf.buf[addr];
					if(half) b=(b&0xf0)|(hv&0xf);
					else b=(b&0xf)|((hv&0xf)<<4);
					fbuf.buf[addr]=b;
					unsaved=true;
				}
				else if(addr==fbuf.i)
				{
					append_char(&fbuf, half?hv&0xf:(hv&0xf)<<4);
					unsaved=true;
				}
				draw_title(file, fbuf, unsaved, oldsize);
				half++;
				if(half>1)
				{
					half=0;
					cursx++;
					if(cursx>=hcols)
					{
						cursx-=hcols;
						goto kdown;
					}
					if((scroll+cursy)*hcols+cursx>fbuf.i)
						cursx--;
					curs_status(cursy, cursx, hcols, scroll);
				}
			}
		}
		else
		{
			half=0;
			unsigned int addr=(scroll+cursy)*hcols+cursx;
			switch(key)
			{
				case 5: // C-e = toggle endianness
					lendian=!lendian;
					status(lendian?"Little Endian mode selected":"Big Endian mode selected");
				break;
				case 16: // C-p = write out a coPy
					free(file);
					file=NULL;
					/* fallthrough */
				case 15: // C-o = write Out
					if(!file)
					{
						string fn=init_string();
						int key;
						do
						{
							char st[cols];
							if(fn.i+10>cols)
								snprintf(st, cols, "Save as: >%s", fn.buf+fn.i+11-cols);
							else
								snprintf(st, cols, "Save as: %s", fn.buf);
							status(st);
							key=getch();
							if((key>=32)&&(key<127)) append_char(&fn, key);
							else if(key==KEY_BACKSPACE) fn.buf[fn.i=max(fn.i, 1)-1]=0;
						} while(!((key==KEY_ENTER)||(key==13)));
						file=fn.buf;
					}
					if(file)
					{
						FILE *f=fopen(file, "wb");
						if(!f)
						{
							char st[cols];
							if(strlen(strerror(errno))+strlen(file)+33<cols)
								sprintf(st, "he: file '%s' could not be opened: %s", file, strerror(errno));
							else if(strlen(strerror(errno))+30<cols)
								sprintf(st, "he: file could not be opened: %s", strerror(errno));
							else
								sprintf(st, "fopen: %s", strerror(errno));
							status(st);
						}
						else
						{
							for(unsigned int i=0;i<fbuf.i;i++)
								fputc(fbuf.buf[i], f);
							fclose(f);
							char st[32];
							sprintf(st, "Wrote out %zu bytes%s", fbuf.i, fbuf.i==oldsize?"":"*");
							status(st);
							unsaved=false;
						}
						oldsize=fbuf.i;
						draw_title(file, fbuf, unsaved, oldsize);
					}
				break;
				case 6: // C-f = load a File
					free(file);
					file=NULL;
					/* fallthrough */
				case 18: // C-r = Read file
					if(unsaved)
					{
						status("Unsaved!  Are you sure? [y/n]");
						if(tolower(getch())!='y')
						{
							status("Cancelled read");
							break;
						}
					}
					if(!file)
					{
						string fn=init_string();
						int key;
						do
						{
							char st[cols];
							if(fn.i+12>cols)
								snprintf(st, cols, "Read from: >%s", fn.buf+fn.i+13-cols);
							else
								snprintf(st, cols, "Read from: %s", fn.buf);
							status(st);
							key=getch();
							if((key>=32)&&(key<127)) append_char(&fn, key);
							else if(key==KEY_BACKSPACE) fn.buf[fn.i=max(fn.i, 1)-1]=0;
						} while(!((key==KEY_ENTER)||(key==13)));
						file=fn.buf;
					}
					if(file)
					{
						FILE *f=fopen(file, "rb");
						if(!f)
						{
							char st[cols];
							if(strlen(strerror(errno))+strlen(file)+33<cols)
								sprintf(st, "he: file '%s' could not be opened: %s", file, strerror(errno));
							else if(strlen(strerror(errno))+30<cols)
								sprintf(st, "he: file could not be opened: %s", strerror(errno));
							else
								sprintf(st, "fopen: %s", strerror(errno));
							status(st);
						}
						else
						{
							if(!(fbuf=sslurp(f)).buf) file=NULL;
							unsaved=false;
							status("Ready");
						}
						oldsize=fbuf.i;
						draw_title(file, fbuf, unsaved, oldsize);
					}
				break;
				case 24: // C-x = exit
					if(unsaved)
					{
						status("Unsaved!  Are you sure? [y/n]");
						if(tolower(getch())!='y')
						{
							status("Cancelled");
							break;
						}
					}
					errupt++;
				break;
				case KEY_IC:
					if(addr>fbuf.i) status("Can't insert - cursor is in outer space");
					else
					{
						append_char(&fbuf, 0);
						for(unsigned int i=fbuf.i;i>addr;i--)
							fbuf.buf[i]=fbuf.buf[i-1];
						fbuf.buf[addr]=0;
						draw_title(file, fbuf, unsaved=true, oldsize);
					}
				break;
				case KEY_DC:
					if(addr>fbuf.i) status("Can't delete - cursor is in outer space");
					else if(addr==fbuf.i) status("Nothing to delete");
					else if(!fbuf.i) status("Nothing to delete");
					else
					{
						fbuf.i--;
						for(unsigned int i=addr;i<fbuf.i;i++)
							fbuf.buf[i]=fbuf.buf[i+1];
						draw_title(file, fbuf, unsaved=true, oldsize);
					}
				break;
				case KEY_PPAGE:
					if(scroll)
						scroll=max(scroll, scrolljump)-scrolljump;
					else
						cursy=cursx=0;
					curs_status(cursy, cursx, hcols, scroll);
				break;
				case KEY_NPAGE:
					cursy+=scrolljump-1;
					goto kdown;
				case KEY_UP:
					kup:
					if(cursy) cursy--;
					else
					{
						if(scroll)
						{
							if(scroll>=scrolljump)
							{
								scroll-=scrolljump;
								cursy=scrolljump-1;
							}
							else
							{
								cursy=scroll-1;
								scroll=0;
							}
						}
						else
							cursx=0;
					}
					curs_status(cursy, cursx, hcols, scroll);
				break;
				case KEY_DOWN:
					kdown:
					cursy++;
					if((scroll+cursy)*hcols+cursx>fbuf.i)
					{
						while((scroll+cursy)*hcols>fbuf.i)
							cursy--;
						cursx=fbuf.i-(scroll+cursy)*hcols;
					}
					if(cursy>=rows-irows-2)
					{
						scroll+=scrolljump;
						cursy-=scrolljump;
					}
					curs_status(cursy, cursx, hcols, scroll);
				break;
				case KEY_LEFT:
					if(cursx) cursx--;
					else
					{
						cursx=hcols-1;
						goto kup;
					}
					curs_status(cursy, cursx, hcols, scroll);
				break;
				case KEY_RIGHT:;
					int eat=1;
					#if INFO_Z80_DISASSEMBLER
					if(display[zinf]&&(zeat>0)&&(zeat<hcols)) eat=zeat;
					#endif
					cursx+=eat;
					if(cursx>=hcols)
					{
						cursx-=hcols;
						goto kdown;
					}
					if((scroll+cursy)*hcols+cursx>fbuf.i)
						cursx--;
					curs_status(cursy, cursx, hcols, scroll);
				break;
				case 9: // tab = switch panes
					left=!left;
				break;
				case KEY_F(1):
					if(f1cycle>=ninfos) f1cycle=0;
					if(ninfos)
					{
						if(infos[f1cycle].name)
						{
							char st[16+strlen(infos[f1cycle].name)];
							sprintf(st, "F%d: info %s", f1cycle+2, infos[f1cycle].name);
							status(st);
						}
						f1cycle++;
						if(f1cycle>=ninfos) f1cycle=0;
					}
				break;
				default:
					if((key>=KEY_F(2))&&(key<(signed)KEY_F(2+ninfos)))
					{
						int i=key-KEY_F(2);
						bool d=display[i]=!display[i];
						if(infos[i].name)
						{
							char st[16+strlen(infos[i].name)];
							sprintf(st, "%s info %s", d?"Enabled":"Disabled", infos[i].name);
							status(st);
						}
					}
					else
					{
						if(!left && (key<256))
						{
							unsigned int addr=(scroll+cursy)*hcols+cursx;
							if(addr<fbuf.i)
							{
								fbuf.buf[addr]=(unsigned char)key;
								unsaved=true;
							}
							else if(addr==fbuf.i)
							{
								append_char(&fbuf, (unsigned char)key);
								unsaved=true;
							}
							draw_title(file, fbuf, unsaved, oldsize);
							cursx++;
							if(cursx>=hcols)
							{
								cursx-=hcols;
								goto kdown;
							}
							if((scroll+cursy)*hcols+cursx>fbuf.i)
								cursx--;
							curs_status(cursy, cursx, hcols, scroll);
						}
						else
						{
							const char *name=key_name(key);
							if(name)
							{
								char st[20+strlen(name)];
								sprintf(st, "No binding for key %s", name);
								status(st);
							}
						}
					}
				break;
			}
		}
	}
	endwin();
	return(EXIT_SUCCESS);
}

void status(const char *st)
{
	unsigned int rows, cols;
	getmaxyx(stdscr, rows, cols);
	attron(A_BOLD);
	mvprintw(rows-1, 0, "%.*s", cols, st);
	unsigned int y, x;
	getyx(stdscr, y, x);
	for(;y<cols;y++)
		addch(' ');
	attroff(A_BOLD);
}

void curs_status(unsigned int y, unsigned int x, unsigned int hcols, unsigned int scroll)
{
	char st[64];
	snprintf(st, 64, "(%hu,%hu) = 0x%08x = %u", y, x, (scroll+y)*hcols+x, (scroll+y)*hcols+x);
	status(st);
}

int initialise_curses(void)
{
	initscr();
	if(cbreak()==ERR)
	{
		endwin();
		fprintf(stderr, "he: initialise_curses: cbreak() call failed\n");
		return(1);
	}
	if(noecho()==ERR)
	{
		endwin();
		fprintf(stderr, "he: initialise_curses: noecho() call failed\n");
		return(1);
	}
	if(nonl()==ERR)
	{
		endwin();
		fprintf(stderr, "he: initialise_curses: nonl() call failed\n");
		return(1);
	}
	intrflush(stdscr, false);
	if(keypad(stdscr, true)==ERR)
	{
		endwin();
		fprintf(stderr, "he: initialise_curses: keypad() call failed\n");
		return(1);
	}
	curs_set(0);
	return(0);
}

void draw_title(const char *file, string fbuf, bool unsaved, size_t oldsize)
{
	unsigned int rows, cols;
	getmaxyx(stdscr, rows, cols);
	string titlebar=make_string(" he");
	append_str(&titlebar, " "VERSION);
	if(file && (titlebar.i+strlen(file)+3<cols))
	{
		append_str(&titlebar, " \"");
		append_str(&titlebar, file);
		append_str(&titlebar, "\"");
	}
	if(unsaved)
		append_char(&titlebar, '*');
	if(fbuf.i)
	{
		char bytes[32];
		snprintf(bytes, 32, "%zu", fbuf.i);
		if(titlebar.i+strlen(bytes)+9<cols)
		{
			append_str(&titlebar, " (");
			append_str(&titlebar, bytes);
			append_str(&titlebar, " bytes)");
		}
		if(fbuf.i!=oldsize) append_char(&titlebar, '*');
	}
	attron(A_REVERSE);
	mvprintw(0, 0, "%s", titlebar.buf);
	unsigned int y, x;
	getyx(stdscr, y, x);
	for(;x<cols;x++)
		addch(' ');
	attroff(A_REVERSE);
	free_string(&titlebar);
}

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
#include <curses.h>
#include <locale.h>
#include <errno.h>

#include "bits.h"
#include "keynames.h"
#include "infos.h"

#define VERSION	"0.0.1"

int initialise_curses(void);
void status(const char *st);
void curs_status(unsigned int y, unsigned int x, unsigned int hcols, unsigned int scroll);

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	const char *file=NULL; // laziness and single file, for now
	for(int arg=1;arg<argc;arg++)
	{
		file=argv[arg];
	}
	string fbuf=null_string();
	if(file)
	{
		FILE *f=fopen(file, "r");
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
	}
	attron(A_REVERSE);
	mvprintw(0, 0, "%s", titlebar.buf);
	unsigned int y, x;
	getyx(stdscr, y, x);
	for(;x<cols;x++)
		addch(' ');
	attroff(A_REVERSE);
	free_string(&titlebar);
	unsigned int hcols=(cols-13)/4;
	status("Ready");
	unsigned int scroll=0, cursy=0, cursx=0;
	bool left=true;
	unsigned int f1cycle=0;
	lendian=false;
	int errupt=0;
	while(!errupt)
	{
		unsigned int irows=1+countirows((cursy+scroll)*hcols+cursx, fbuf, cols);
		unsigned int scrolljump=(rows-irows-2)/2;
		render_irows((cursy+scroll)*hcols+cursx, fbuf, cols, rows-irows);
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
						attron(left?A_STANDOUT|A_BOLD:A_BOLD);
					mvprintw(y, (x*3)+11, "%02x", c);
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
						mvprintw(y, (x*3)+11, "  ");
					else
						mvaddch(y, x+(hcols*3)+13, ' ');
					attroff(A_STANDOUT);
					if(!left)
						mvprintw(y, (x*3)+11, "  ");
					else
						mvaddch(y, x+(hcols*3)+13, ' ');
				}
			}
			if(x<hcols) break;
		}
		int key=getch();
		/*if((key>=32)&&(key<127))
		{
			status("Error: editing not done yet");
		}
		else*/
		{
			switch(key)
			{
				case 5: // C-e = toggle endianness
					lendian=!lendian;
					status(lendian?"Little Endian mode selected":"Big Endian mode selected");
				break;
				case 24: // C-x = exit
					errupt++;
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
				case KEY_RIGHT:
					cursx++;
					if(cursx>=hcols)
					{
						cursx=0;
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
					if(infos[f1cycle].name)
					{
						char st[16+strlen(infos[f1cycle].name)];
						sprintf(st, "F%d: info %s", f1cycle+2, infos[f1cycle].name);
						status(st);
					}
					f1cycle++;
					if(f1cycle>=ninfos) f1cycle=0;
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
						const char *name=key_name(key);
						if(name)
						{
							char st[20+strlen(name)];
							sprintf(st, "No binding for key %s", name);
							status(st);
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

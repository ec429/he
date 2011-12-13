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

#define VERSION	"0.0.1"

typedef enum
{
	C_NORMAL=1,
}
colour;

int initialise_curses(void);
void initialise_colours(void);

void status(const char *st);

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
	initialise_colours();
	attron(COLOR_PAIR(C_NORMAL));
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
	status("Ready");
	int errupt=0;
	while(!errupt)
	{
		int key=getch();
		if((key>=32)&&(key<127))
		{
			status("Error: editing not done yet");
		}
		else
		{
			switch(key)
			{
				case 24: // C-x = exit
					errupt++;
				break;
				default:
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
	start_color();
	curs_set(1);
	return(0);
}

void initialise_colours(void)
{
	init_pair(C_NORMAL, COLOR_WHITE, COLOR_BLACK);
}

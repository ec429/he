#include "keynames.h"

char knbuf[4]={0};

const char *key_name(int k)
{
	if((k>32)&&(k<127))
	{
		knbuf[0]=k;
		knbuf[1]=0;
		return(knbuf);
	}
	if((k>0)&&(k<32))
	{
		knbuf[0]='^';
		knbuf[1]=k+0x40;
		knbuf[2]=0;
		return(knbuf);
	}
	if((k>=KEY_F(0))&&(k<=KEY_F(63)))
	{
		knbuf[0]='F';
		snprintf(knbuf+1, 3, "%d", k-KEY_F0);
		return(knbuf);
	}
	switch(k)
	{
		case 32:
			return("space");
		case KEY_BREAK:
			return("break");
		case KEY_DOWN:
			return("down");
		case KEY_UP:
			return("up");
		case KEY_LEFT:
			return("left");
		case KEY_RIGHT:
			return("right");
		case KEY_HOME:
			return("home");
		case KEY_BACKSPACE:
			return("backspace");
		case KEY_DL:
			return("delete line");
		case KEY_IL:
			return("insert line");
		case KEY_DC:
			return("delete char");
		case KEY_IC:
			return("insert char");
		case KEY_EIC:
			return("exit insert char mode");
		case KEY_CLEAR:
			return("clear screen");
		case KEY_EOS:
			return("clear to end of screen");
		case KEY_EOL:
			return("clear to end of line");
		case KEY_SF:
			return("scroll forward");
		case KEY_SR:
			return("scroll reverse");
		case KEY_NPAGE:
			return("next page");
		case KEY_PPAGE:
			return("prev page");
		case KEY_ENTER:
			return("enter");
		case KEY_END:
			return("end");
		case KEY_MOUSE:
			return("mouse event!");
		case KEY_RESIZE:
			return("screen resize event!");
		default:
			return("(unrecognised key)");
	}
}

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include "bits.h"
#include "utf8.h"
char **split(char *, char *, int);
uni_pt *add_uni_pt(uni_pt, uint32_t *, uni_pt **);

#if INFO_UNICODE_UTF8
size_t uwrite(uint32_t val, char *buf);
size_t uread(const char *str, uint32_t *val, size_t length);

int init_unicode(void)
{
	if(upts) return(1);
	upts=0;
	utbl=NULL;
	FILE *udata=fopen("/usr/share/i18n/charmaps/UTF-8", "r");
	if(!udata) return(-1);
	while(!feof(udata))
	{
		char *line=fgetl(udata);
		if(!line)
		{
			fclose(udata);
			return(-1);
		}
		if(strcmp(line, "CHARMAP")==0)
		{
			free(line);
			break;
		}
		free(line);
	}
	while(!feof(udata))
	{
		char *line=fgetl(udata);
		if(!line) break;
		if(strcmp(line, "END CHARMAP")==0)
		{
			free(line);
			break;
		}
		if(*line)
		{
			char **fields=split(line, " ", 3);
			if(!fields)
				return(-1);
			uni_pt new;
			new.name=strdup(fields[2]);
			new.isdup=false;
			if(strchr(fields[0], '.'))
			{
				bool hit=add_uni_pt(new, &upts, &utbl);
				uint32_t last;
				sscanf(fields[0], "<U%x>..<U%x>", &new.code, &last);
				for(uint32_t i=new.code+1;i<=last;i++)
				{
					if(add_uni_pt((uni_pt){.code=i, .name=new.name, .isdup=hit}, &upts, &utbl))
						hit=true;
				}
				free(fields);
				if(!hit)
					free(new.name);
			}
			else
			{
				sscanf(fields[0], "<U%x>", &new.code);
				free(fields);
				if(!add_uni_pt(new, &upts, &utbl))
					free(new.name);
			}
		}
		free(line);
	}
	fclose(udata);
	return(0);
}

size_t uread(const char *str, uint32_t *val, size_t length) // returns number of bytes 'eaten'
{
	if(!str) return(0);
	if(!length) return(0);
	if(*str&0x80)
	{
		if(*str&0x40)
		{
			if(*str&0x20)
			{
				if(*str&0x10)
				{
					if(*str&0x08) // 11111xxx, error
					{
						if(val) *val=0xdc80|(*str&0x7f);
						return(1);
					}
					else // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
					{
						if((length>=4)&&((str[1]&0xc0)==0x80)&&((str[2]&0xc0)==0x80)&&((str[3]&0xc0)==0x80))
						{
							if(val) *val=((str[0]&0x0f)<<18)|((str[1]&0x3f)<<12)|((str[2]&0x3f)<<6)|(str[3]&0x3f);
							return(4);
						}
						else
						{
							if(val) *val=0xdc80|(*str&0x7f);
							return(1);
						}
					}
				}
				else // 1110xxxx 10xxxxxx 10xxxxxx
				{
					if((length>=3)&&((str[1]&0xc0)==0x80)&&((str[2]&0xc0)==0x80))
					{
						if(val) *val=((str[0]&0x0f)<<12)|((str[1]&0x3f)<<6)|(str[2]&0x3f);
						return(3);
					}
					else
					{
						if(val) *val=0xdc80|(*str&0x7f);
						return(1);
					}
				}
			}
			else // 110xxxxx 10xxxxxx
			{
				if((length>=2)&&((str[1]&0xc0)==0x80))
				{
					if(val) *val=((str[0]&0x1f)<<6)|(str[1]&0x3f);
					return(2);
				}
				else
				{
					if(val) *val=0xdc80|(*str&0x7f);
					return(1);
				}
			}
		}
		else // error, is a continuation byte
		{
			if(val) *val=0xdc80|(*str&0x7f);
			return(1);
		}
	}
	else
	{
		if(val) *val=*str;
		return(1);
	}
}

size_t uwrite(uint32_t val, char *buf) // returns number of bytes emitted (if !buf, number of bytes needed)
{
	if(val<0x80)
	{
		if(buf)
			buf[0]=val;
		return(1);
	}
	else if(val<0x800) // 110x xxyy  10yy yyyy
	{
		if(buf)
		{
			buf[0]=0xc0|(char)(val>>6);
			buf[1]=0x80|(char)(val&0x3f);
		}
		return(2);
	}
	else if(val<0xffff) // 1110 xxxx  10xx xxyy  10yy yyyy
	{
		if(buf)
		{
			buf[0]=0xe0|(char)(val>>12);
			buf[1]=0x80|(char)((val>>6)&0x3f);
			buf[2]=0x80|(char)(val&0x3f);
		}
		return(3);
	}
	else if(val<0x10ffff) // 1111 0xxx  10xx yyyy  10yy yyzz  10zz zzzz
	{
		if(buf)
		{
			buf[0]=0xe0|(char)(val>>18);
			buf[1]=0x80|(char)((val>>12)&0x3f);
			buf[2]=0x80|(char)((val>>6)&0x3f);
			buf[3]=0x80|(char)(val&0x3f);
		}
		return(4);
	}
	else
	{
		// �, 0xefbfbd
		if(buf)
		{
			buf[0]=(char)0xef;
			buf[1]=(char)0xbf;
			buf[2]=(char)0xbd;
		}
		return(3);
	}
}

char **split(char *line, char *fs, int count)
{
	char **rv=malloc(count*sizeof(char *));
	if(!rv)
		return(NULL);
	for(int n=0;n<count;n++)
	{
		size_t pref=strcspn(line, fs);
		rv[n]=line;
		if(n<count-1)
		{
			if(!line[pref])
			{
				free(rv);
				return(NULL);
			}
			line[pref]=0;
			line+=pref+1;
			line+=strspn(line, fs);
		}
	}
	return(rv);
}

uni_pt *add_uni_pt(uni_pt new, uint32_t *n, uni_pt **p)
{
	uint32_t o=*n;
	uni_pt *np=realloc(*p, ++*n*sizeof(uni_pt));
	if(!np)
	{
		*n=0;
		return(NULL);
	}
	(*p=np)[o]=new;
	return(np+o);
}

int render_utf8(unsigned int addr, string bytes, bool draw, int maxw)
{
	if(!upts)
		return(0);
	uint32_t code;
	size_t s=uread(bytes.buf+addr, &code, bytes.i-addr);
	if(!s)
		return(0);
	int len=(code>0xFFFF)?8:4;
	if(maxw<9+len) return(0);
	size_t w=0;
	const char *name="Invalid codepoint";
	uint32_t l=0, r=code, off=(l+r)>>1, ooff=0;
	bool odd=false;
	while(utbl[off].code!=code)
	{
		if(utbl[off].code<code)
			l=off;
		else
			r=off;
		off=(l+r)>>1;
		if(off==ooff)
		{
			if(odd)
			{
				off=-1;
				break;
			}
			else
			{
				off++;
				odd=true;
			}
		}
		ooff=off;
	}
	if(off!=(uint32_t)-1)
	{
		name=utbl[off].name;
		w=strlen(name);
	}
	ueat=s;
	if(!name||(maxw<12+len+(signed)strlen(name)))
	{
		if(draw)
			printw("UTF8: U+%0*x ", len, code);
		return(9+len);
	}
	else
	{
		if(draw)
			printw("UTF8: U+%0*x = %s ", len, code, name);
		return(12+len+strlen(name));
	}
}
#endif

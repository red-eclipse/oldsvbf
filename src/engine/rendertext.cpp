#include "pch.h"
#include "engine.h"

static hashtable<const char *, font> fonts;
static font *fontdef = NULL;

font *curfont = NULL;

void newfont(char *name, char *tex, int *defaultw, int *defaulth, int *offsetx, int *offsety, int *offsetw, int *offseth)
{
	font *f = fonts.access(name);
	if(!f)
	{
		name = newstring(name);
		f = &fonts[name];
		f->name = name;
	}

	f->tex = textureload(tex);
	f->chars.setsize(0);
	f->defaultw = *defaultw;
	f->defaulth = *defaulth;
	f->offsetx = *offsetx;
	f->offsety = *offsety;
	f->offsetw = *offsetw;
	f->offseth = *offseth;

	fontdef = f;
}

void fontchar(int *x, int *y, int *w, int *h)
{
	if(!fontdef) return;

	font::charinfo &c = fontdef->chars.add();
	c.x = *x;
	c.y = *y;
	c.w = *w ? *w : fontdef->defaultw;
	c.h = *h ? *h : fontdef->defaulth;
}

COMMANDN(font, newfont, "ssiiiiii");
COMMAND(fontchar, "iiii");

bool setfont(const char *name)
{
	font *f = fonts.access(name);
	if(!f) return false;
	curfont = f;
	return true;
}

void gettextres(int &w, int &h)
{
	if(w < MINRESW || h < MINRESH)
	{
		if(MINRESW > w*MINRESH/h)
		{
			h = h*MINRESW/w;
			w = MINRESW;
		}
		else
		{
			w = w*MINRESH/h;
			h = MINRESH;
		}
	}
}

#define PIXELTAB (8*curfont->defaultw)

int text_width(const char *str) { //@TODO deprecate in favour of text_bounds(..)
    int width, height;
    text_bounds(str, width, height);
    return width;
}

void draw_textf(const char *fstr, int left, int top, ...)
{
	s_sprintfdlv(str, top, fstr);
	draw_text(str, left, top);
}

static int draw_char(int c, int x, int y)
{
    font::charinfo &info = curfont->chars[c-33];
    float tc_left    = (info.x + curfont->offsetx) / float(curfont->tex->xs);
    float tc_top     = (info.y + curfont->offsety) / float(curfont->tex->ys);
    float tc_right   = (info.x + info.w + curfont->offsetw) / float(curfont->tex->xs);
    float tc_bottom  = (info.y + info.h + curfont->offseth) / float(curfont->tex->ys);

    glTexCoord2f(tc_left,  tc_top   ); glVertex2i(x,          y);
    glTexCoord2f(tc_right, tc_top   ); glVertex2i(x + info.w, y);
    glTexCoord2f(tc_right, tc_bottom); glVertex2i(x + info.w, y + info.h);
    glTexCoord2f(tc_left,  tc_bottom); glVertex2i(x,          y + info.h);

    xtraverts += 4;
    return info.w;
}

//stack[sp] is current color index
static void text_color(char c, char *stack, int size, int &sp, bvec color, int a)
{
    if(c=='s') // save color
    {
        c = stack[sp];
        if(sp<size-1) stack[sp++] = c;
    }
    else
    {
        if(c=='r') c = stack[(sp > 0) ? --sp : sp]; // restore color
        else stack[sp] = c;
        switch(c)
        {
            case 'g': case '0': color = bvec( 64, 255, 128); break;   // green: player talk
            case 'b': case '1': color = bvec( 96, 160, 255); break;   // blue: "echo" command
            case 'y': case '2': color = bvec(255, 192,  64); break;   // yellow: gameplay messages
            case 'r': case '3': color = bvec(255,  64,  64); break;   // red: important errors
            case 'G': case '4': color = bvec(128, 128, 128); break;   // gray
            case 'm': case '5': color = bvec(192,  64, 192); break;   // magenta
            case 'o': case '6': color = bvec(255, 128,   0); break;   // orange
            // white (provided color): everything else
        }
        glColor4ub(color.x, color.y, color.z, a);
    }
}

#define TEXTSKELETON \
    int y = 0, x = 0;\
    int i;\
    for(i = 0; str[i]; i++)\
    {\
        TEXTINDEX(i)\
        int c = str[i];\
        if(c=='\t')      { x = ((x+PIXELTAB)/PIXELTAB)*PIXELTAB; TEXTWHITE(i) }\
        else if(c==' ')  { x += curfont->defaultw; TEXTWHITE(i) }\
        else if(c=='\n') { TEXTLINE(i) x = 0; y += FONTH; }\
        else if(c=='\f') { if(str[i+1]) { i++; TEXTCOLOR(i) }}\
        else if(curfont->chars.inrange(c-33))\
        {\
            if(maxwidth != -1)\
            {\
                int j = i;\
                int w = curfont->chars[c-33].w;\
                for(; str[i+1]; i++)\
                {\
                    int c = str[i+1];\
                    if(c=='\f') { if(str[i+2]) i++; continue; }\
                    if(i-j > 16) break;\
                    if(!curfont->chars.inrange(c-33)) break;\
                    int cw = curfont->chars[c-33].w + 1;\
                    if(w + cw >= maxwidth) break;\
                    w += cw;\
                }\
                if(x + w >= maxwidth && j!=0) { TEXTLINE(j-1) x = 0; y += FONTH; }\
                TEXTWORD\
            }\
            else\
            { TEXTCHAR(i) }\
        }\
    }

//all the chars are guaranteed to be either drawable or color commands
#define TEXTWORDSKELETON \
                for(; j <= i; j++)\
                {\
                    TEXTINDEX(j)\
                    int c = str[j];\
                    if(c=='\f') { if(str[j+1]) { j++; TEXTCOLOR(j) }}\
                    else { TEXTCHAR(j) }\
                }

int text_visible(const char *str, int hitx, int hity, int maxwidth)
{
    #define TEXTINDEX(idx)
    #define TEXTWHITE(idx) if(y+FONTH > hity && x >= hitx) return idx;
    #define TEXTLINE(idx) if(y+FONTH > hity) return idx;
    #define TEXTCOLOR(idx)
    #define TEXTCHAR(idx) x += curfont->chars[c-33].w+1; TEXTWHITE(idx)
    #define TEXTWORD TEXTWORDSKELETON
    TEXTSKELETON
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
    return i;
}

//inverse of text_visible
void text_pos(const char *str, int cursor, int &cx, int &cy, int maxwidth)
{
    #define TEXTINDEX(idx) if(idx == cursor) { cx = x; cy = y; break; }
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx)
    #define TEXTCOLOR(idx)
    #define TEXTCHAR(idx) x += curfont->chars[c-33].w + 1;
    #define TEXTWORD TEXTWORDSKELETON if(i >= cursor) break;
    cx = INT_MIN;
    cy = 0;
    TEXTSKELETON
    if(cx == INT_MIN) { cx = x; cy = y; }
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
}

void text_bounds(const char *str, int &width, int &height, int maxwidth)
{
    #define TEXTINDEX(idx)
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx) if(x > width) width = x;
    #define TEXTCOLOR(idx)
    #define TEXTCHAR(idx) x += curfont->chars[c-33].w + 1;
    #define TEXTWORD x += w + 1;
    width = 0;
    TEXTSKELETON
    height = y + FONTH;
    TEXTLINE(_)
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
}

int colorpos = 0;
char colorstack[10];

void draw_text(const char *str, int rleft, int rtop, int r, int g, int b, int a, bool s, int cursor, int maxwidth)
{
    #define TEXTINDEX(idx) if(idx == cursor) { cx = x; cy = y; }
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx)
    #define TEXTCOLOR(idx) text_color(str[idx], colorstack, sizeof(colorstack), colorpos, color, a);
    #define TEXTCHAR(idx) x += draw_char(c, left+x, top+y)+1;
    #define TEXTWORD TEXTWORDSKELETON
    bvec color(r, g, b);
    int cx = INT_MIN, cy = 0, left = rleft, top = rtop;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, curfont->tex->id());
    glBegin(GL_QUADS);
	loopk(s ? 2 : 1)
	{
		if (s && !k)
		{
			glColor4ub(0, 0, 0, a);
			left = rleft+2;
			top = rtop+2;
		}
		else
		{
			glColor4ub(color.x, color.y, color.z, a);
			left = rleft;
			top = rtop;
		}
		TEXTSKELETON
		if(cursor >= 0 && (totalmillis/250)&1)
		{
			glColor4ub(r, g, b, a);
			if(cx == INT_MIN) { cx = x; cy = y; }
			if(maxwidth != -1 && cx >= maxwidth) { cx = 0; cy += FONTH; }
			draw_char('_', left+cx, top+cy);
		}
	}
    glEnd();
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
}

void reloadfonts()
{
    enumerate(fonts, font, f,
        if(!reloadtexture(*f.tex)) fatal("failed to reload font texture");
    );
}


void draw_textx(const char *fstr, int left, int top, int r, int g, int b, int a, bool s, int align, int cursor, int maxwidth, ...)
{
	s_sprintfdlv(str, maxwidth, fstr);

	int x = left, y = top, width = text_width(str);

	switch (align)
	{
		case AL_CENTER:
			x -= width/2;
			break;
		case AL_RIGHT:
			x -= width;
			break;
		default:
			break;
	}
	draw_text(str, x, y, r, g, b, a, s, cursor, maxwidth);
}

static vector<font *> fontstack;

bool pushfont(const char *name)
{
	if (curfont) fontstack.add(curfont);
	return setfont(name);
}

bool popfont(int num)
{
	int n = fontstack.length();

	loopi(num)
	{
		if (n <= 0) break;
		fontstack.remove(--n);
	}
	return (n != fontstack.length());
}

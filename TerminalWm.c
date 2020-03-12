#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include "config.h"
/*data types*/


typedef struct MyScreen MyScreen;
typedef struct Win Win;
struct MyScreen
{
	Display *dpy;	
	Window rootwin;
	int screen_num;
	int s_width, s_height; //display_width and height
};
struct Win
{
	Window win;
	Atom tag_number; /*the number that appears on top of each window*/
	Atom mode;/*the mode that a window is under*/
	Atom status;/*---only in cursorless mode---the master/stack status of a window*/
	int width, height;
	GC gc;
	XftColor scheme[2];
	XftFont *font;
	
};
enum {Foreground, Background};

MyScreen *screen;
int mode;
/*function prototypes*/
void init();
void eventLoop();
void drawBar(int mode);
static XftFont *xft_font_alloc(MyScreen *screen, const char *fontname);
void xft_color_alloc(MyScreen *screen, XftColor *color, const char *colorname);
void drawRect(MyScreen *screen, Win *win,GC gc, int x, int y, unsigned int w, unsigned int h, int filled);
void drawTextCenter(MyScreen *screen, Win *win, const char *text);
void buttonPress(XEvent *);
void configureRequest(XEvent *);
void configureNotify(XEvent *);
void destroyNotify(XEvent *);
void enterNotify(XEvent *);
void expose(XEvent *);
void focusIn(XEvent *);
void keyPress(XEvent *);
void mappingNotify(XEvent *);
void mapRequest(XEvent *);
void motionNotify(XEvent *);
void unmapNotify(XEvent *);
/*function designated array initializer*/
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonPress,
	[ConfigureRequest] = configureRequest,
	[ConfigureNotify] = configureNotify,
	[DestroyNotify] = destroyNotify,
	[EnterNotify] = enterNotify,
	[Expose] = expose,
	[FocusIn] = focusIn,
	[KeyPress] = keyPress,
	[MappingNotify] = mappingNotify,
	[MapRequest] = mapRequest,
	[MotionNotify] = motionNotify,
	[UnmapNotify] = unmapNotify
};
static XftFont *
xft_font_alloc(MyScreen *screen, const char *fontname)
{
	XftFont *font = NULL;

	if (fontname && screen) {
			if (!(font = XftFontOpenName(screen->dpy, screen->screen_num, fontname))) {
			fprintf(stderr, "error with name '%s'\n", fontname);
			return NULL;
		}
	}
	return font;
}
void
xft_color_alloc(MyScreen *screen, XftColor *color, const char *colorName)
{
	if (!screen || !color || !colorName)
		return;

	if (!XftColorAllocName(screen->dpy, DefaultVisual(screen->dpy, screen->screen_num),
	                       DefaultColormap(screen->dpy, screen->screen_num),
	                       colorName, color))
		fprintf(stderr,"error, cannot allocate color '%s'", colorName);
}
void
drawRect(MyScreen *screen, Win *win,GC gc, int x, int y, unsigned int w, unsigned int h, int filled)
{
	if (!win)
		return;
	if (filled)
		XFillRectangle(screen->dpy, win->win, gc, x, y, w, h);
	else
		XDrawRectangle(screen->dpy, win->win, gc, x, y, w - 1, h - 1);
}
void drawTextCenter(MyScreen *screen, Win *win, const char *text)
{
	if(!win || !screen)
		return;

	XGlyphInfo *glyph=NULL;
	XftDraw *d= XftDrawCreate(screen->dpy, win->win,
		                  DefaultVisual(screen->dpy, screen->screen_num),
		                  DefaultColormap(screen->dpy, screen->screen_num));

	XftTextExtentsUtf8(screen->dpy, win->font, (const FcChar8 *)text, strlen(text), glyph);
	XftColor col=win->scheme[Foreground];
	int width=(win->width-glyph->width)/2;
	int height=(win->height-glyph->height)/2;
	XftDrawStringUtf8(d,&col,win->font, width,height,( XftChar8 *)text, strlen(text));
		
}
void init()
{
	screen->dpy=XOpenDisplay(NULL);
	screen->screen_num=DefaultScreen(screen->dpy);
	screen->rootwin=RootWindow(screen->dpy, screen->screen_num);
	screen->s_width=DisplayWidth(screen->dpy, screen->screen_num);
	screen->s_height=DisplayHeight(screen->dpy, screen->screen_num);
	XSelectInput(screen->dpy, screen->rootwin, SubstructureRedirectMask|SubstructureNotifyMask);

	drawBar(mode);	
}
void eventloop()
{
	XEvent ev;
	XNextEvent(screen->dpy, &ev);
	for(;;){
		handler[ev.type](&ev);
	}
}
void drawBar(int mode)
{
	Window bar=XCreateSimpleWindow(screen->dpy, screen->rootwin,0,0, screen->s_width, 15, 1,BlackPixel(screen->dpy,
			 screen->screen_num),WhitePixel(screen->dpy, screen->screen_num));
	XMapWindow(screen->dpy, bar);
	
}
void buttonPress(XEvent *ev)
{
}
void configureRequest(XEvent *ev)
{
}
void configureNotify(XEvent *ev)
{
}
void destroyNotify(XEvent *ev)
{
}
void enterNotify(XEvent *ev)
{
}
void expose(XEvent *ev)
{
}
void focusIn(XEvent *ev)
{
}
void keyPress(XEvent *ev)
{
}
void mappingNotify(XEvent *ev)
{
}
void mapRequest(XEvent *ev)
{
}
void motionNotify(XEvent *ev)
{
}
void unmapNotify(XEvent *ev)
{
}
int main()
{
	init();
	eventloop();
	return 0;
	
}

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "config.h"
/*data types*/
struct Screen
{
	Display *dpy;	
	Window rootwin;
	int screen_num;
	int s_width, s_height; //display_width and height
}
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
	
}
struct {Foreground, Background};
typedef struct Screen Screen;
typedef struct Win Win;
Screen *screen;
int mode;
/*function prototypes*/
void init();
void eventLoop();
void drawBar(int mode);
static XftFont *xftFontAlloc(Screen *screen, const char *fontname);
void xftColorAlloc(Screen *screen, XftColor *color, const char *colorname);

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
/*function array initializer*/
static void (*handler[event]) (XEvent *) = {
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
xftFontAlloc(Screen *screen, const char *fontname)
{
	XftFont *font = NULL;

	if (fontname && screen) {
			if (!(xfont = XftFontOpenName(screen->dpy, screen->screen_num, fontname))) {
			fprintf(stderr, "error with name '%s'\n", fontname);
			return NULL;
		}
	}
	return xfont;
}
void
xftCollorAlloc(Screen *screen, XftColor *color, const char *colorName)
{
	if (!screen || !color || !colorname)
		return;

	if (!XftColorAllocName(screen->dpy, DefaultVisual(screen->dpy, screen->screen_num),
	                       DefaultColormap(screen->dpy, screen->screen_num),
	                       colorname, color))
		fprintf(stderr,"error, cannot allocate color '%s'", colorName);
}
void
drawRect(Screen *screen, Win *win,GC gc, int x, int y, unsigned int w, unsigned int h, int filled)
{
	if (!win)
		return;
	if (filled)
		XFillRectangle(screen->dpy, win->win, gc, x, y, w, h);
	else
		XDrawRectangle(screen->dpy, win->win, gc, x, y, w - 1, h - 1);
}
void drawText(Screen *screen, Win *win, int x, int y, const char *text)
{
	if(!win || !screen)
		return;
	XGlyphInfo *glyph=NULL;
	XftDraw *d= XftDrawCreate(screen->dpy, Win->win,
		                  DefaultVisual(screen->dpy, screen->screen_num),
		                  DefaultColormap(screen->dpy, screen->screen_num));
	XftTextExtentsUtf8(d, win->font, (const FcChar8 *)text, strlen(text), &glyph);
	XftColor *col=win->scheme[Foreground];
	XftDrawStringUtf8(d,col,win->font, , ty, (XftChar8 *)buf, len);
		
}
void setup()
{
	screen->dpy=XOpenDisplay(NULL);
	screen->screen_num=DefaultScreen(screen->dpy);
	screen->rootwin=RootWindow(screen->dpy, screen->screen_num);
	screen->s_with=DisplayWidth(screen->dpy, screen->screen_num);
	screen->s_height=DisplayHeight(screen->dpy, screen->screen_num);
	XSelectInput(screen->dpy, screen->rootWin, SubstructureRedirectMask|SubstructureNotifyMask);	
}
void eventloop()
{
	XEvent ev;
	for(;;){
		handler[ev.type](&ev);
	}
}
void drawBar(int mode)
{
	GC 
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

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <X11/Xproto.h>
/*macros*/
#define LENGTH(X) (sizeof X/ sizeof X[0])
#define MAX(a, b) ((a) > (b) ? (a) : (b))
/*data types*/


typedef struct MyScreen MyScreen;
typedef struct Win Win;
typedef struct Key Key;
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
	int x,y;
	int width, height;
	GC gc;
	XftColor scheme[2];
	XftFont *font;
	
};
struct Key
{
	unsigned int mod;//key mask
	KeySym keysym;//key sym
	void (*func)(XEvent *);//array designated intializers
};
enum {Foreground, Background};


MyScreen *screen;
XWindowAttributes attr;
XButtonEvent start;
int mode;
XftFont *font=NULL;
/*function prototypes*/
void init();
void eventLoop();
void checkOtherWM();
int wmError(Display *, XErrorEvent *);
void drawBar();
int xerror(Display *, XErrorEvent *);
static int (*xerrorxlib)(Display *, XErrorEvent *);

static XftFont *xft_font_alloc(MyScreen *, const char *);
void xft_color_alloc(MyScreen *, XftColor *, const char *);
void drawRect(MyScreen *screen, Win *,GC, int, int, unsigned int, unsigned int, int);
void drawTextCenter(MyScreen *, Win *, const char *);

void buttonPress(XEvent *);
void buttonRelease(XEvent *);
void configureRequest(XEvent *);
void configureNotify(XEvent *);
void createTerm(XEvent *);
void createNotify(XEvent *);
void destroyNotify(XEvent *);
void destroyWin(XEvent *);
void enterNotify(XEvent *);
void expose(XEvent *);
void focusIn(XEvent *);
void keyPress(XEvent *);
void mappingNotify(XEvent *);
void mapRequest(XEvent *);
void motionNotify(XEvent *);
void switchToCursorless(XEvent *);
void switchToCursor(XEvent *);
void switchToMaster(XEvent *);
void switchToSlave(XEvent *);
void unmapNotify(XEvent *);

#include "config.h"

/*function designated array initializer*/
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonPress,
	[ButtonRelease]=buttonRelease,
	[ConfigureRequest] = configureRequest,
	[ConfigureNotify] = configureNotify,
	[CreateNotify]=createNotify,
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

	XGlyphInfo *glyph=malloc(sizeof(XGlyphInfo));
	XftDraw *d= XftDrawCreate(screen->dpy, win->win,
		                  DefaultVisual(screen->dpy, screen->screen_num),
		                  DefaultColormap(screen->dpy, screen->screen_num));

	XftTextExtentsUtf8(screen->dpy, win->font, (const FcChar8 *)text, strlen(text), glyph);
	XftColor col=win->scheme[Foreground];
	int width=(win->width-glyph->width)/2;
	int height=(win->height-glyph->height)/2+glyph->height;
	XftDrawStringUtf8(d,&col,win->font, width,height,( XftChar8 *)text, strlen(text));
}
void init()
{
	screen=malloc(sizeof(MyScreen));
	screen->dpy=XOpenDisplay(NULL);
	screen->screen_num=DefaultScreen(screen->dpy);
	screen->rootwin=RootWindow(screen->dpy, screen->screen_num);
	screen->s_width=DisplayWidth(screen->dpy, screen->screen_num);
	screen->s_height=DisplayHeight(screen->dpy, screen->screen_num);
	checkOtherWM();
	XSelectInput(screen->dpy, screen->rootwin, SubstructureRedirectMask|SubstructureNotifyMask);
	
	font=xft_font_alloc(screen, g_font);
/*kinda not my problem if other apps use these key bindings*/

	XGrabKey(screen->dpy, XKeysymToKeycode(screen->dpy, 0x31), Mod1Mask,
            screen->rootwin, True, GrabModeAsync, GrabModeAsync);	
	XGrabKey(screen->dpy, XKeysymToKeycode(screen->dpy, 0x32), Mod1Mask,
            screen->rootwin, True, GrabModeAsync, GrabModeAsync);
	
	XGrabButton(screen->dpy, 1, Mod1Mask,screen->rootwin, True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    	XGrabButton(screen->dpy, 3, Mod1Mask,screen->rootwin, True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

	drawBar();	
}
void checkOtherWM()
{
	XSetErrorHandler(wmError);	
	XSelectInput(screen->dpy, screen->rootwin, SubstructureRedirectMask|SubstructureNotifyMask);
	XSync(screen->dpy, False);
	XSetErrorHandler(xerror);
}

int xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); 
}

int wmError(Display *dpy, XErrorEvent *ev)
{
	fprintf(stderr, "another wm is already running");
	exit(1);
	return -1;
}
void createNotify(XEvent *ev)
{

/*nothing for the wm to do with a CreateNotify event */

}
void eventloop()
{

	XEvent ev;
	for(;;){
		XNextEvent(screen->dpy, &ev);
		if(handler[ev.type])
			handler[ev.type](&ev);
	}
}
void drawBar()
{
	Colormap cmap = DefaultColormap(screen->dpy, screen->screen_num);
	XColor col;
	XColor active_col;
	XParseColor(screen->dpy,cmap,"rgb:88/88/88",&col);
	XAllocColor(screen->dpy, cmap, &col);	
	XParseColor(screen->dpy, cmap, active_mode_col, &active_col);
	XAllocColor(screen->dpy, cmap, &active_col);
	
	Window bar=XCreateSimpleWindow(screen->dpy, 
					screen->rootwin,0,0, screen->s_width, 20, 0, BlackPixel(screen->dpy, screen->screen_num),col.pixel);

	Win *cursorless_mode=malloc(sizeof(Win));
	Window cursorless_mode_win=XCreateSimpleWindow(screen->dpy, 
					screen->rootwin,0,0,20,20, 0,BlackPixel(screen->dpy, screen->screen_num), mode? col.pixel:active_col.pixel); 
	cursorless_mode->win=cursorless_mode_win;
	cursorless_mode->width=20;
	cursorless_mode->height=20;
	XftColor cursorless_foreground;
	xft_color_alloc(screen,&cursorless_foreground,"#ffffff");
	cursorless_mode->scheme[Foreground]=cursorless_foreground;
	cursorless_mode->font=font;

	Win *cursor_mode=malloc(sizeof(Win));
	Window cursor_mode_win=XCreateSimpleWindow(screen->dpy, screen->rootwin, 20,0,20,20,0,BlackPixel(screen->dpy, screen->screen_num), mode? active_col.pixel:col.pixel);
	cursor_mode->win=cursor_mode_win;
	cursor_mode->width=20;
	cursor_mode->height=20;
 	XftColor cursor_foreground;
	xft_color_alloc(screen, &cursor_foreground, "#ffffff");
	cursor_mode->scheme[Foreground]=cursor_foreground;
	cursor_mode->font=font;

	XMapWindow(screen->dpy, bar);
	XMapWindow(screen->dpy, cursorless_mode->win);
	XMapWindow(screen->dpy, cursor_mode->win);
	drawTextCenter(screen,cursorless_mode,"1");
	drawTextCenter(screen,cursor_mode,"2");
	
	
}
void buttonPress(XEvent *ev)
{
	printf("called\n");
	XButtonEvent *event=&ev->xbutton;
	if(mode==1){
		if(event->subwindow != None)
        	{
            		XGetWindowAttributes(screen->dpy, event->subwindow, &attr);
            		start = *event;
			
        	}
	}
}
void buttonRelease(XEvent *ev)
{
	XButtonEvent *event=&ev->xbutton;

	if(mode==1)
		start.subwindow=None;		

}
void configureRequest(XEvent *ev)
{
/*Configure requests are called at window startups. Allow the request by invoking a configure request ourselves*/
	XConfigureRequestEvent *event;
	event=&ev->xconfigurerequest;
	XWindowChanges changes;
  // Copy fields from event to changes.
	changes.x = event->x;
	changes.y = event->y;
	changes.width = event->width;
	changes.height = event->height;
	changes.border_width = event->border_width;
	changes.sibling = event->above;
	changes.stack_mode = event->detail;
 
	XConfigureWindow(screen->dpy, event->window, event->value_mask, &changes);
}
void configureNotify(XEvent *ev)
{
}
void createTerm(XEvent *ev)
{
}
void destroyNotify(XEvent *ev)
{
}
void destroyWin(XEvent *ev)
{
}
void enterNotify(XEvent *ev)
{
}
void expose(XEvent *ev)
{
	drawBar();
}
void focusIn(XEvent *ev)
{
}
void keyPress(XEvent *ev)
{
	XKeyEvent *event=&ev->xkey;
	KeySym keysym = XKeycodeToKeysym(screen->dpy, (KeyCode)event->keycode, 0);
	for (int i = 0; i < LENGTH(keys); i++)
	{
		if (keysym == keys[i].keysym
		&& (keys[i].mod) == (event->state)
		&& keys[i].func)
			keys[i].func(ev);
	}	
}
void mappingNotify(XEvent *ev)
{
}
void mapRequest(XEvent *ev)
{
	XMapRequestEvent *event;
	event=&ev->xmaprequest;
/*reparenting with our own window*/

/*allow the map request*/
	XMapWindow(screen->dpy, event->window);
}
void motionNotify(XEvent *ev){
	XMotionEvent *event=&ev->xmotion;
	int xdiff;
	int ydiff;
	if(mode==1)
	{
		if(start.subwindow != None)
		{
			xdiff = (event->x_root) - (start.x_root);
			ydiff = (event->y_root) - (start.y_root);
			XMoveResizeWindow(screen->dpy, start.subwindow,
				attr.x + (start.button==1 ? xdiff : 0),
				attr.y + (start.button==1 ? ydiff : 0),
				MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
				MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
		}
	}
}
void switchToCursorless(XEvent *ev)
{
	mode=0;
	drawBar();
}
void switchToCursor(XEvent *ev)
{
	mode=1;
	drawBar();
}
void switchToMaster(XEvent *ev)
{
}
void switchToSlave(XEvent *ev)
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

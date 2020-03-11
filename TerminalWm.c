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
typedef struct Screen Screen;
Screen *screen;
/*function prototypes*/
void init();
void eventLoop();
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
int main()
{
	init();
	eventloop();
	return 0;
	
}

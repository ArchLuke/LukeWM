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
#define MIN(a,b)  ((a)<(b)?(a):(b));
/*data types*/


typedef struct MyScreen MyScreen;
typedef struct Win Win;
typedef struct Key Key;
struct MyScreen
{
	Display *dpy;	
	Window rootwin;
	Colormap cmap;
	int screen_num;
	int s_width, s_height; //display_width and height
};
struct Win
{
	char name[5];
	Window win;
/*
I made mode, tag_number, and status of type Atom for POTENTIAL future inter-client communications. 
I am imagining writing other helper X applications, specifically made for TerminalWM, that further enhance the usability of my DM. These applications may require the mode, tag_number, and status of itself or any other windows.

*/
	Atom mode;
	Atom tag_number; /*---only in cursorless mode---the number that appears on top of each window*/
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

/*colors*/
XColor active_col;
XColor bar_color;
XColor par_col;
XftColor white;
XftColor black;
/*Win related datas*/
Win *cursorless_mode;//the bar
Win *cursor_mode;//the bar
Window *cursor_wins;
int cursor_length=0;//number of entries in cursor_wins;
Win **cursorless_wins;
int cursorless_length=0;//number of entries in cursorless_wins;

int mode=1;
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
void leaveNotify(XEvent *);
void mappingNotify(XEvent *);
void mapRequest(XEvent *);
void motionNotify(XEvent *);
void reparentWin(Window);
void switchToCursorless(XEvent *);
void switchToCursor(XEvent *);
void switchToMaster(XEvent *);
void switchToSlave(XEvent *);
void updateCursorList(Window win);
void updateCursorLessList(Win *win);
void unmapNotify(XEvent *);
void thisIsATrollFunctionPrototype();
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
	[LeaveNotify] = leaveNotify,
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
	                       screen->cmap,
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
		                  screen->cmap);

	XftTextExtentsUtf8(screen->dpy, win->font, (const FcChar8 *)text, strlen(text), glyph);
	XftColor col=win->scheme[Foreground];
	int width=(win->width-glyph->width)/2;
	int height=(win->height-glyph->height)/2+glyph->height;
	XftDrawStringUtf8(d,&col,win->font, width,height,( XftChar8 *)text, strlen(text));
	//free(glyph);
}
void init()
{
		
	screen=malloc(sizeof(MyScreen));
	cursor_wins=calloc(10, sizeof(Window));
	cursorless_wins=calloc(10,sizeof(void *));
	screen->dpy=XOpenDisplay(NULL);
	screen->screen_num=DefaultScreen(screen->dpy);
	screen->rootwin=RootWindow(screen->dpy, screen->screen_num);
	screen->s_width=DisplayWidth(screen->dpy, screen->screen_num);
	screen->s_height=DisplayHeight(screen->dpy, screen->screen_num);
	screen->cmap = DefaultColormap(screen->dpy, screen->screen_num);

	XParseColor(screen->dpy, screen->cmap, active_mode_col, &active_col);
	XAllocColor(screen->dpy, screen->cmap, &active_col);
	XParseColor(screen->dpy, screen->cmap, bar_col, &bar_color);
	XAllocColor(screen->dpy, screen->cmap,  &bar_color);
	XParseColor(screen->dpy, screen->cmap, parent_col, &par_col);
	XAllocColor(screen->dpy, screen->cmap,  &par_col);
	xft_color_alloc(screen, &white, "#ffffff");
	xft_color_alloc(screen, &black,"#000000");


	checkOtherWM();
	XSelectInput(screen->dpy, screen->rootwin, SubstructureRedirectMask|SubstructureNotifyMask|ExposureMask);
	
	font=xft_font_alloc(screen, g_font);
/*kinda not my problem if other apps use these key bindings*/

	XGrabKey(screen->dpy, XKeysymToKeycode(screen->dpy, 0x31), Mod1Mask,
            screen->rootwin, True, GrabModeAsync, GrabModeAsync);	
	XGrabKey(screen->dpy, XKeysymToKeycode(screen->dpy, 0x32), Mod1Mask,
            screen->rootwin, True, GrabModeAsync, GrabModeAsync);

//	XGrabButton(screen->dpy, 1,None,screen->rootwin, True,
  //          ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
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
	XSetWindowBackground(screen->dpy, cursor_mode->win, WhitePixel(screen->dpy, screen->screen_num));
	XEvent ev;
	for(;;){
		XNextEvent(screen->dpy, &ev);
		if(handler[ev.type])
			handler[ev.type](&ev);
	}
}
void drawBar()
{
	
	Window bar=XCreateSimpleWindow(screen->dpy, 
					screen->rootwin,0,0, screen->s_width, 20, 0, BlackPixel(screen->dpy, screen->screen_num),bar_color.pixel);
	cursorless_mode=malloc(sizeof(Win));
	Window cursorless_mode_win=XCreateSimpleWindow(screen->dpy, 
					screen->rootwin,0,0,20,20, 0,BlackPixel(screen->dpy, screen->screen_num), mode? bar_color.pixel:active_col.pixel);
	cursorless_mode->win=cursorless_mode_win;
	cursorless_mode->scheme[Foreground]=white;
	cursorless_mode->font=font;
	cursorless_mode->width=20;
	cursorless_mode->height=20;

	cursor_mode=malloc(sizeof(Win));
	Window cursor_mode_win=XCreateSimpleWindow(screen->dpy, screen->rootwin, 20,0,20,20,0,BlackPixel(screen->dpy, screen->screen_num), mode? active_col.pixel:bar_color.pixel);
	cursor_mode->win=cursor_mode_win;
	cursor_mode->scheme[Foreground]=white;
	cursor_mode->font=font;
	cursor_mode->width=20;
	cursor_mode->height=20;
	
	XMapWindow(screen->dpy, bar);
	XMapWindow(screen->dpy, cursorless_mode->win);
	XMapWindow(screen->dpy, cursor_mode->win);
	drawTextCenter(screen,cursorless_mode,"1");
	drawTextCenter(screen,cursor_mode,"2");

	
	
}
void buttonPress(XEvent *ev)
{
	XButtonEvent *event=&ev->xbutton;
	if(mode==1){
		Window root;
		Window parent;
		Window *childwins;
		int number;
		XQueryTree(screen->dpy,event->window,&root,&parent,&childwins,&number);
		if(event->subwindow != None)
        	{
			/*raising and updating the pointer focus of the clicked window*/
			XRaiseWindow(screen->dpy, event->subwindow);
			XSetInputFocus(screen->dpy,event->subwindow,RevertToParent, CurrentTime);

			if(event->state==MODEMASK)
			{
						
            			XGetWindowAttributes(screen->dpy, event->subwindow, &attr);
            			start = *event;
			}
			
        	}else if(number !=0){
		/*handling normal clicking events*/
	
			XRaiseWindow(screen->dpy, event->window);
			XSetInputFocus(screen->dpy,event->window, RevertToParent,CurrentTime);	
		}else{
		/*when the close button is pressed*/
			Window root;
			Window parent;
			Window *childwins;
			int number;
/*getting and shutting down the reparenting window, which in turn shuts down the main window*/
			XQueryTree(screen->dpy,event->window,&root,&parent,&childwins,&number);
			if(parent)
				XDestroyWindow(screen->dpy,parent);
			updateCursorList(parent);
		}
	}
}
void buttonRelease(XEvent *ev)
{
	XButtonEvent *event=&ev->xbutton;

	if(mode==1 && event->subwindow!=None)
		start.subwindow=None;		

}
void configureRequest(XEvent *ev)
{
	
	/*Configure requests are called at window startups. Allow the request by invoking a configure request ourselves*/
	XConfigureRequestEvent *event;
	event=&ev->xconfigurerequest;
	XWindowChanges changes={};
  // Copy fields from event to changes.
	changes.x = event->x;
//making sure the window does not go out of bounds
	changes.y = ((event->y)>40)?(event->y):40;

	changes.width = event->width;
	changes.height = event->height;
	changes.border_width = event->border_width;
	changes.sibling = event->above;
	changes.stack_mode = event->detail;
 	
	XConfigureWindow(screen->dpy, event->window,CWX | CWY | CWWidth | CWHeight, &changes);
}
void configureNotify(XEvent *ev)
{

}
void createTerm(XEvent *ev)
{

}
void destroyNotify(XEvent *ev)
{
	printf("destroyed\n");
}
void destroyWin(XEvent *ev)
{

}
void enterNotify(XEvent *ev)
{/*
	XCrossingEvent *event=&ev->xcrossing;
	XUnmapWindow(screen->dpy,event->window);
	XSetWindowBackground(screen->dpy,event->window, WhitePixel(screen->dpy,screen->screen_num));
	XMapWindow(screen->dpy, event->window);
	Win *target=malloc(sizeof(Window));
	target->win=event->window;
	target->font=font;
	target->width=20;
	target->height=20;
	target->scheme[Foreground]=black;
//	drawTextCenter(screen, target,"X");*/

	XFlush(screen->dpy);
	
			
}
void expose(XEvent *ev)
{
	if(mode==1)
	{
		for(int i=0;i<cursor_length;i++)
		{
			Window rootwin;
			Window parentwin;
			Window *childwins;
			int number;
			XQueryTree(screen->dpy,cursor_wins[i],&rootwin, &parentwin, &childwins, &number);
			Win *childwin;
			childwin->win=childwins[0];	
			childwin->font=font;
			childwin->width=20;
			childwin->height=20;
			XftColor foreground;
			xft_color_alloc(screen, &foreground, "#000000");
			childwin->scheme[Foreground]=foreground;
			XClearWindow(screen->dpy,childwin->win);
			drawTextCenter(screen, childwin,"X");
		}
	}	
	
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
void leaveNotify(XEvent *ev)
{
//	XCrossingEvent *event=&ev->xcrossing;
//	XSetWindowBackground(screen->dpy,event->window, par_col.pixel);

}
void mappingNotify(XEvent *ev)
{
}
void mapRequest(XEvent *ev)
{
	XMapRequestEvent *event;
	event=&ev->xmaprequest;
/*reparenting with our own window, adding close buttons and etc.*/
	reparentWin(event->window);

/*allow the map request*/
	XMapWindow(screen->dpy, event->window);
}
void motionNotify(XEvent *ev){
	XMotionEvent *event=&ev->xmotion;
	int xdiff, ydiff;
	int new_x=start.x_root;
	int new_y=start.y_root;
	int new_width=attr.width;
	int  new_height=attr.height;
	if(mode==1)
	{
		if(start.subwindow != None)
		{
			xdiff = (event->x_root) - (start.x_root);
			ydiff = (event->y_root) - (start.y_root);
			
			new_x=MAX((attr.x + (start.button==1 ? xdiff : 0)),0); /*left boundary*/
			new_x=MIN((screen->s_width-attr.width),new_x);/*right boundary*/
			new_y=MAX((attr.y + (start.button==1 ? ydiff : 0)),20);/*upper bound*/
			new_y=MIN((screen->s_height-attr.height),new_y);/*lower bound*/

			new_width=MAX(40, attr.width + (start.button==3 ? xdiff : 0));/*minimum width*/
			new_height=MAX(40, attr.height + (start.button==3 ? ydiff : 0));/*minimum height*/
			new_width=((attr.x+new_width)>screen->s_width)?(screen->s_width-attr.x):new_width;/*maximum width*/	
			new_height=((attr.y+new_height)>screen->s_height)?(screen->s_height-attr.y):new_height;/*maximum width*/
			
			XMoveResizeWindow(screen->dpy, start.subwindow,
				new_x,
				new_y,	
				new_width,
				new_height);

			/*resizing childwins*/

			Window *childwin;
			Window parentWin;
			Window rootwin;
			int number;
			XQueryTree(screen->dpy,start.subwindow,&rootwin,&parentWin, &childwin,&number);
			XMoveWindow(screen->dpy, childwin[0],new_width-20, 0);
			XResizeWindow(screen->dpy,childwin[1], new_width, new_height-20);
		}
	}
}
void reparentWin(Window win)
{
/*Getting the attributes of win*/

	XWindowAttributes attr;
	XGetWindowAttributes(screen->dpy, win, &attr);

	Window parent_win=XCreateSimpleWindow(screen->dpy,screen->rootwin, attr.x, attr.y-20,attr.width, attr.height+20,1,BlackPixel(screen->dpy, screen->screen_num),par_col.pixel);
	if(mode==1)
	{
		cursor_wins[cursor_length]=parent_win;
		cursor_length++;
		
		Window cross_win=XCreateSimpleWindow(screen->dpy, parent_win,attr.width-20,0,20,20, 1, BlackPixel(screen->dpy, screen->screen_num),par_col.pixel);
	/* we care when the cursor enters the window or when the window is pressed*/

		XSelectInput(screen->dpy,parent_win, ButtonPressMask);
		XSelectInput(screen->dpy,cross_win, EnterWindowMask | LeaveWindowMask | ButtonPressMask | ExposureMask);
		XSetWindowAttributes attrs;
		attrs.do_not_propagate_mask=ButtonPressMask;
		XChangeWindowAttributes(screen->dpy,cross_win,CWDontPropagate,&attrs);
		XMapWindow(screen->dpy, cross_win);

	}else{
		Win *parent=malloc(sizeof(Win));
		parent->win=parent_win;
		parent->font=font;
		parent->scheme[0]=white;
	}
	XReparentWindow(screen->dpy,win,parent_win,0,20);

	XMapWindow(screen->dpy, parent_win);

			
}
void switchToCursorless(XEvent *ev)
{
	mode=0;
	XUnmapWindow(screen->dpy, cursor_mode->win);
	XUnmapWindow(screen->dpy, cursorless_mode->win);
	XSetWindowBackground(screen->dpy,cursorless_mode->win,active_col.pixel);
	XSetWindowBackground(screen->dpy, cursor_mode->win, bar_color.pixel);
	XMapWindow(screen->dpy, cursor_mode->win);
	XMapWindow(screen->dpy, cursorless_mode->win);
	drawTextCenter(screen,cursorless_mode,"1");
	drawTextCenter(screen,cursor_mode,"2");

	XFlush(screen->dpy);
}
void switchToCursor(XEvent *ev)
{
	mode=1;
	XUnmapWindow(screen->dpy, cursor_mode->win);
	XUnmapWindow(screen->dpy, cursorless_mode->win);
	XSetWindowBackground(screen->dpy,cursor_mode->win,active_col.pixel);
	XSetWindowBackground(screen->dpy, cursorless_mode->win, bar_color.pixel);
	XMapWindow(screen->dpy, cursor_mode->win);
	XMapWindow(screen->dpy, cursorless_mode->win);
	drawTextCenter(screen,cursorless_mode,"1");
	drawTextCenter(screen,cursor_mode,"2");

	XFlush(screen->dpy);

}
void switchToMaster(XEvent *ev)
{

}
void switchToSlave(XEvent *ev)
{

}
void updateCursorLessList(Win *win)
{

} 
void updateCursorList(Window win)
{
	int i=0;
	for(i;i<cursor_length;i++)
	{
		if(cursor_wins[i]==win)
			break;
	}
	for(i;i<cursor_length-1;i++)
	{
		cursor_wins[i]=cursor_wins[i+1];
	}
	cursor_wins[i+1]=None;
	cursor_length --;
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

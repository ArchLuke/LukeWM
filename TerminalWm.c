#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>

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
	int width, height;
	XftColor scheme[2];
	XftFont *font;
	
};
struct Key
{
	unsigned int mod;//key mask
	KeySym keysym;//key sym
	void (*func)(XEvent *);//function for each key
};
enum {Foreground, Background};

MyScreen *screen;

XWindowAttributes attr;
XButtonEvent start;

/*appearances*/
XColor active_col;
XColor bar_color;
XColor par_col;
XftColor white;
XftColor black;
XftFont *font=NULL;

/*Window related datas*/

Win *cursorless_mode;//the bar
Win *cursor_mode;//the bar
Window *cursor_wins;//This list will also be used for future features such as minimizing (using XA_WM_NAME)
int cursor_length=0;//number of entries in cursor_wins;
Window *cursorless_wins;
int cursorless_length=0;//number of entries in cursorless_wins;
int mode=1;

/*atoms*/
Atom utf8_string;
/*
I made status of type Atom for POTENTIAL future inter-client communications. 
I am imagining writing other helper X applications, specifically made for TerminalWM, that further enhance the usability of my DM. These applications may require the master stack statuses.

*/
Atom status;/*the master/stack status of a window
The number is 0 if the window is in cursor mode*/

/*function prototypes*/

void addToCursorLessList(Window win);
void addToCursorList(Window win);
void drawBar();
void drawTextCenter(MyScreen *, Win *, const char *);
void buttonPress(XEvent *);
void buttonRelease(XEvent *);
void checkOtherWM();
void configureRequest(XEvent *);
void createTerm(XEvent *);
void defineCursor(Window win);
void destroyMaster();
void destroyWin(XEvent *);
void enterNotify(XEvent *);
void eventLoop();
void expose(XEvent *);
void init();
void keyPress(XEvent *);
void leaveNotify(XEvent *);
void mapRequest(XEvent *);
void motionNotify(XEvent *);
void moveLeft(XEvent *);
void moveRight(XEvent *);
void organizeCursorLessList();
void reparentWin(Window);
void switchToCursor(XEvent *);
void switchToCursorless(XEvent *);
void switchToMaster(XEvent *);
void switchToStack(XEvent *);
void tileStack();
void removeFromCursorList(Window win);
void removeFromCursorLessList(Window win);
int wmError(Display *, XErrorEvent *);
int xerror(Display *, XErrorEvent *);
static int (*xerrorxlib)(Display *, XErrorEvent *);
void xft_color_alloc(MyScreen *, XftColor *, const char *);
static XftFont *xft_font_alloc(MyScreen *, const char *);
#include "config.h"

/*function designated array initializer*/
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonPress,
	[ButtonRelease]=buttonRelease,
	[ConfigureRequest] = configureRequest,
	[EnterNotify] = enterNotify,
	[LeaveNotify] = leaveNotify,
	[Expose] = expose,
	[KeyPress] = keyPress,
	[MapRequest] = mapRequest,
	[MotionNotify] = motionNotify
};
void addToCursorList(Window win)
{
	cursor_wins[cursor_length]=win;
	cursor_length ++;

}
void addToCursorLessList(Window win)
{
	cursorless_wins[cursorless_length]=win;
	cursorless_length++;
}
void drawBar()
{
	/*draw the bar*/
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
void drawTextCenter(MyScreen *screen, Win *win, const char *text)
{
	if(!win || !screen)
		return;

	XGlyphInfo glyph;
	XftDraw *d= XftDrawCreate(screen->dpy, win->win,
		                  DefaultVisual(screen->dpy, screen->screen_num),
		                  screen->cmap);
/*The text extents of a text measures the attributes of the bounding box.
Visit https://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html to learn about glyph metrics*/
	XftTextExtentsUtf8(screen->dpy, win->font, (const FcChar8 *)text, strlen(text), &glyph);
	XftColor col=win->scheme[Foreground];

	int width=(win->width-glyph.width)/2;
	int height=(win->height-glyph.height)/2+glyph.height;
/*The XftDrawStringUtf8 is a pain in the ass. Note that (x,y) measures NOT the top left corner but the bottom left.
 This is counter-intuitive but also never mentioned in the manual*/
	XftDrawStringUtf8(d,&col,win->font, width,height,( XftChar8 *)text, strlen(text));
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
			removeFromCursorList(parent);
		}
	}
}
void buttonRelease(XEvent *ev)
{
	XButtonEvent *event=&ev->xbutton;

	if(mode==1 && event->subwindow!=None)
		start.subwindow=None;		

}

void checkOtherWM()
{
/*If another wm is already running. A SubstructureRedirect request will fail*/
	XSetErrorHandler(wmError);	
	XSelectInput(screen->dpy, screen->rootwin, SubstructureRedirectMask|SubstructureNotifyMask);
	XSync(screen->dpy, False);
	XSetErrorHandler(xerror);
}


void configureRequest(XEvent *ev)
{
	
	/*Configure requests are called at window startups. Allow the request by invoking a configure request ourselves*/
	XConfigureRequestEvent *event;
	event=&ev->xconfigurerequest;
	XWindowChanges changes;
  // Copy fields from event to changes.
	changes.x = event->x;
//making sure the window does not go out of bounds
	changes.y = ((event->y)>(showbar?40:20))?(event->y):(showbar?40:20);
	if(mode==1)
	{
		changes.width = event->width;
		changes.height = event->height;
	}else{
		/*master pane attributes*/
		changes.x=0;
		changes.y=(showbar?40:20);
		changes.width=screen->s_width/2;
		changes.height=screen->s_height-(showbar?40:20);
	}
	changes.border_width = event->border_width;
	changes.sibling = event->above;
	changes.stack_mode = event->detail;
 	
	XConfigureWindow(screen->dpy, event->window,CWX | CWY | CWWidth | CWHeight, &changes);
}

void createTerm(XEvent *ev)
{
	system("xterm&");
}

void defineCursor(Window win)
{
	Cursor cur=XCreateFontCursor(screen->dpy,cursor);
	XDefineCursor(screen->dpy,win,cur);
}

void destroyMaster()
{
	if(cursorless_length<2)
		return;

	Window new_master=cursorless_wins[cursorless_length-1];
	/*giving the new master window a master property, as it was a stack window before*/

	XChangeProperty(screen->dpy, new_master, status,utf8_string,8,
							PropModeReplace,(unsigned char*) "master",6);
	XWindowChanges attr;
	attr.x=0;
	attr.y=(showbar?20:0);
	attr.width=screen->s_width/2;
	attr.height=(screen->s_height)-(showbar?20:0);
	XConfigureWindow(screen->dpy,new_master,CWX | CWY | CWWidth | CWHeight,&attr);
	/*retile the stack as well*/

	tileStack();		
				
}
void destroyWin(XEvent *ev)
{
	if(mode==0)
	{
		XKeyEvent *event=&ev->xkey;
		/*testing the status of the destroyed window, whether it is a master or a stack window*/

		int di;
		unsigned long dl;
		unsigned char *p;
		Atom da, atom;
		XGetWindowProperty(screen->dpy, event->subwindow, status, 0L, sizeof atom, False, utf8_string,
		&da, &di, &dl, &dl, &p);

		XDestroyWindow(screen->dpy, event->subwindow);
		/*remove the window from the list*/

		removeFromCursorLessList(event->subwindow);
		/*if it is a stack window, re-tile the stack windows*/

		if(strcmp(p,"stack")==0)
			tileStack();
		else
			destroyMaster();/*if it is a master window, destroy the current master window and bring a new one*/
	}
}
void enterNotify(XEvent *ev)
{
	XCrossingEvent *event=&ev->xcrossing;
/*changing the background color if hovered*/
	XSetWindowBackground(screen->dpy,event->window, WhitePixel(screen->dpy,screen->screen_num));
	XClearWindow(screen->dpy,event->window);
	XDrawLine(screen->dpy,event->window, DefaultGC(screen->dpy, screen->screen_num),0,0,20,20);
	XDrawLine(screen->dpy,event->window, DefaultGC(screen->dpy, screen->screen_num),20,0,0,20);

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
void expose(XEvent *ev)
{
	XExposeEvent *event=&ev->xexpose;
/*Interestingly enough, the only drawn elements that are subject to expose events are the close buttons*/
	if(mode==1)
	{
		XClearWindow(screen->dpy,event->window);
		XDrawLine(screen->dpy,event->window, DefaultGC(screen->dpy, screen->screen_num),0,0,20,20);
		XDrawLine(screen->dpy,event->window, DefaultGC(screen->dpy, screen->screen_num),20,0,0,20);
	}	
	
}

void init()
{
		
	screen=malloc(sizeof(MyScreen));
	cursor_wins=calloc(10, sizeof(Window));
	cursorless_wins=calloc(10,sizeof(Window));
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

/*global atoms initialization*/
	utf8_string=XInternAtom(screen->dpy,"UTF8_STRING", False);
	status=XInternAtom(screen->dpy, "status", False);
	
	checkOtherWM();
/*the gut of the wm*/
	XSelectInput(screen->dpy, screen->rootwin, SubstructureRedirectMask|SubstructureNotifyMask|ExposureMask);
	
	font=xft_font_alloc(screen, g_font);
/*global root grabs*/

	XGrabKey(screen->dpy, XKeysymToKeycode(screen->dpy, XK_S), ShiftMask | Mod1Mask,
            screen->rootwin, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(screen->dpy, XKeysymToKeycode(screen->dpy, XK_d), ShiftMask | Mod1Mask,
            screen->rootwin, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(screen->dpy, XKeysymToKeycode(screen->dpy, XK_f), ShiftMask | Mod1Mask,
            screen->rootwin, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(screen->dpy, XKeysymToKeycode(screen->dpy, XK_a), ShiftMask | Mod1Mask,
            screen->rootwin, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(screen->dpy, XKeysymToKeycode(screen->dpy, XK_t), ShiftMask | Mod1Mask,
            screen->rootwin, True, GrabModeAsync, GrabModeAsync);	
	XGrabKey(screen->dpy, XKeysymToKeycode(screen->dpy, XK_c), ShiftMask | Mod1Mask,
            screen->rootwin, True, GrabModeAsync, GrabModeAsync);	
	XGrabKey(screen->dpy, XKeysymToKeycode(screen->dpy, XK_1), Mod1Mask,
            screen->rootwin, True, GrabModeAsync, GrabModeAsync);	
	XGrabKey(screen->dpy, XKeysymToKeycode(screen->dpy, XK_2), Mod1Mask,
            screen->rootwin, True, GrabModeAsync, GrabModeAsync);

	XGrabButton(screen->dpy, 1, Mod1Mask,screen->rootwin, True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    	XGrabButton(screen->dpy, 3, Mod1Mask,screen->rootwin, True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	if(showbar)
		drawBar();
	defineCursor(screen->rootwin);	
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
	XCrossingEvent *event=&ev->xcrossing;

	XSetWindowBackground(screen->dpy,event->window, par_col.pixel);
	XClearWindow(screen->dpy,event->window);
	XDrawLine(screen->dpy,event->window, DefaultGC(screen->dpy, screen->screen_num),0,0,20,20);
	XDrawLine(screen->dpy,event->window, DefaultGC(screen->dpy, screen->screen_num),20,0,0,20);

}
void mapRequest(XEvent *ev)
{
	XMapRequestEvent *event;
	event=&ev->xmaprequest;
/*reparenting with our own window, adding close buttons and etc.*/
	reparentWin(event->window);
	if(mode==0)
	{
/*if the window is created in cursorless mode, we need to reorganize the windows in the stack mode*/
		tileStack();
	}
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
			/*the next 8 lines make sure that the window stays within boundary*/
			new_x=MAX((attr.x + (start.button==1 ? xdiff : 0)),0); /*left boundary*/
			new_x=MIN((screen->s_width-attr.width),new_x);/*right boundary*/
			new_y=MAX((attr.y + (start.button==1 ? ydiff : 0)),(showbar?20:0));/*upper bound*/
			new_y=MIN((screen->s_height-attr.height),new_y);/*lower bound*/

			new_width=MAX(40, attr.width + (start.button==3 ? xdiff : 0));/*minimum width*/
			new_height=MAX(40, attr.height + (start.button==3 ? ydiff : 0));/*minimum height*/
			new_width=((attr.x+new_width)>screen->s_width)?(screen->s_width-attr.x):new_width;/*maximum width*/	
			new_height=((attr.y+new_height)>screen->s_height)?(screen->s_height-attr.y):new_height;/*maximum height*/
			
			XMoveResizeWindow(screen->dpy, start.subwindow,
				new_x,
				new_y,	
				new_width,
				new_height);

			/*resizing childwins, i.e.the cross button and the actual content window*/

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
void moveLeft(XEvent *ev)
{
	XKeyEvent *event=&ev->xkey;
	Window old_focus;
	int state;
/*obtain the current input focus. We will decide the new input focus based on the current one*/
	XGetInputFocus(screen->dpy,&old_focus,&state);

	Window *focus;
	Window *childwin;
	Window parentWin;
	Window rootwin;
	int number;

	int i=0;
/*the following for loop solves for the index of old_focus in the list.*/
	for(i = 0; i < cursorless_length; i++)
	{
		XQueryTree(screen->dpy, cursorless_wins[i],&rootwin,&parentWin,&focus,&number);
  		if(*focus==old_focus)

			break;
	}
	int new_focus=MIN(i+1,cursorless_length-1);

	XQueryTree(screen->dpy,cursorless_wins[new_focus],&rootwin,&parentWin, &focus,&number);
	printf("%d + %d\n", i,number);
	XSetInputFocus(screen->dpy,*focus,RevertToPointerRoot, CurrentTime);
}
void moveRight(XEvent *ev)
{

	XKeyEvent *event=&ev->xkey;
	Window old_focus;
	int state;
	XGetInputFocus(screen->dpy,&old_focus,&state);

	Window *focus;
	Window *childwin;
	Window parentWin;
	Window rootwin;
	int number;

	int i=0;
	for(i = 0; i < cursorless_length; i++)
	{
		XQueryTree(screen->dpy, cursorless_wins[i],&rootwin,&parentWin,&focus,&number);
  		if(*focus==old_focus)

			break;
	}

	int new_focus=MAX(i-1,0);

	XQueryTree(screen->dpy,cursorless_wins[new_focus],&rootwin,&parentWin, &focus,&number);
	XSetInputFocus(screen->dpy,*focus,RevertToPointerRoot, CurrentTime);

}
/*organizeCursorLessList makes sure that the master window is always on the top of the cursorless_wins list. 
Many functions would not work otherwise, as they assume the position of the master window in the list*/

void organizeCursorLessList()
{
	int di;
	unsigned long dl;
	unsigned char *p;
	Atom da, atom;
	int i=0;

	Window master_window;

	/*looping through the list, testing which window is the master window*/
	for(i;i<cursorless_length;i++)
	{
	
		XGetWindowProperty(screen->dpy, cursorless_wins[i], status, 0L, sizeof atom, False, utf8_string,
	&da, &di, &dl, &dl, &p);

		if(strcmp("master",p)==0)
		{
			master_window=cursorless_wins[i];
			break;
		}
	}
	Window stack_window=cursorless_wins[cursorless_length-1];
/*swapping the windows' locations*/
	cursorless_wins[i]=stack_window;
	cursorless_wins[cursorless_length-1]=master_window;

}
void reparentWin(Window win)
{
/*Getting the attributes of win*/

	XWindowAttributes attr;
	XGetWindowAttributes(screen->dpy, win, &attr);

	Window parent_win=XCreateSimpleWindow(screen->dpy,screen->rootwin, attr.x, attr.y-20,attr.width, attr.height+20,1,BlackPixel(screen->dpy, screen->screen_num),par_col.pixel);
	if(mode==1)
	{
	/*if the window is created in cursor mode, the master stack status of the window is 0, i.e. it does not aply*/
		XChangeProperty(screen->dpy,parent_win,status,utf8_string, 8, PropModeReplace, 
							(unsigned char*)"0", 1);		
		addToCursorList(parent_win);
		
		Window cross_win=XCreateSimpleWindow(screen->dpy, parent_win,attr.width-20,0,20,20, 1, BlackPixel(screen->dpy, screen->screen_num),par_col.pixel);

		XSelectInput(screen->dpy,parent_win, ButtonPressMask);
	/* we care when the cursor enters the window or when the window is pressed*/
		XSelectInput(screen->dpy,cross_win, EnterWindowMask | LeaveWindowMask | ButtonPressMask | ExposureMask);
		XMapWindow(screen->dpy, cross_win);

	}else{
		/*any window that has just been created is put in the master pane*/
		XChangeProperty(screen->dpy,parent_win, status,utf8_string, 8,PropModeReplace, 
							(unsigned char*)"master", 6);

		addToCursorLessList(parent_win);
		
	}
	XReparentWindow(screen->dpy,win,parent_win,0,20);
	XMapWindow(screen->dpy, parent_win);
			
}

void switchToCursor(XEvent *ev)
{
	mode=1;
	if(showbar)
	{
/*XSetWindowBackground works in an intriguing way. It doesn't actually change the background color when called. 
I was stuck on this for a very long time, until someone helped me through a forum. The function only changes future requests. The change is only applied when the window is cleared or redrawn. */
		XSetWindowBackground(screen->dpy,cursor_mode->win,active_col.pixel);
		XSetWindowBackground(screen->dpy, cursorless_mode->win, bar_color.pixel);
		XClearWindow(screen->dpy,cursorless_mode->win);
		XClearWindow(screen->dpy,cursor_mode->win);

		drawTextCenter(screen,cursorless_mode,"1");
		drawTextCenter(screen,cursor_mode,"2");
	}
/*unmapping cursorless windows since we're now in cursor mode*/
	for(int i=0; i<cursorless_length;i++)
	{
		XUnmapWindow(screen->dpy,cursorless_wins[i]);
	}
/*mapping cursor windows*/
	for(int i=0;i<cursor_length;i++)
	{
		XMapWindow(screen->dpy,cursor_wins[i]);
	}

	XFlush(screen->dpy);

}
void switchToCursorless(XEvent *ev)
{
	mode=0;
	if(showbar)
	{
		XSetWindowBackground(screen->dpy,cursorless_mode->win,active_col.pixel);
		XSetWindowBackground(screen->dpy, cursor_mode->win, bar_color.pixel);
		XClearWindow(screen->dpy,cursorless_mode->win);
		XClearWindow(screen->dpy,cursor_mode->win);
		drawTextCenter(screen,cursorless_mode,"1");
		drawTextCenter(screen,cursor_mode,"2");
	}
	/*unmapping cursor windows since we're now in cursorless mode*/

	for(int i=0;i<cursor_length;i++)
	{
		XUnmapWindow(screen->dpy,cursor_wins[i]);
	}
	/*mapping cursorless wins*/

	for(int i=0; i<cursorless_length;i++)
	{
		XMapWindow(screen->dpy,cursorless_wins[i]);
	}

	XFlush(screen->dpy);
}
void switchToMaster(XEvent *ev)
{
/*first test if the window is already a master. If so, return*/

	XKeyEvent *event=&ev->xkey;
	int di;
	unsigned long dl;
	unsigned char *p;
	Atom da, atom;
	XGetWindowProperty(screen->dpy, event->subwindow, status, 0L, sizeof atom, False, utf8_string,
	&da, &di, &dl, &dl, &p);
	if(strcmp(p,"master")==0)
		return;

/*changing the old master window to a stack property*/

	Window old_master=cursorless_wins[cursorless_length-1];
	XChangeProperty(screen->dpy, old_master, status,utf8_string,8,
							PropModeReplace,(unsigned char*) "stack",5);
/*the window that is pressed is now the new master window*/
	Window new_master=event->subwindow;
	XChangeProperty(screen->dpy, new_master, status,utf8_string,8,
							PropModeReplace,(unsigned char*) "master",6);
/*configuring the attributes of the new master window*/
	XWindowChanges attr;
	attr.x=0;
	attr.y=(showbar?20:0);
	attr.width=screen->s_width/2;
	attr.height=(screen->s_height)-(showbar?20:0);
	XConfigureWindow(screen->dpy,new_master,CWX | CWY | CWWidth | CWHeight,&attr);

	/*reorganize the list after the structure has been messed up*/
	organizeCursorLessList();

/*re-tile the stack windows*/
	tileStack();
	
}
void switchToStack(XEvent *ev)
{
/*first test if the window is already a stack window. If so, return*/

	XKeyEvent *event=&ev->xkey;
	int di;
	unsigned long dl;
	unsigned char *p;
	Atom da, atom;
	/*the XGetWindowProperty retrieves the property of a window using atoms. Note that neither di, dl,da, nor atom is used, but they are required parameters for the function*/
	XGetWindowProperty(screen->dpy, event->subwindow, status, 0L, sizeof atom, False, utf8_string,
	&da, &di, &dl, &dl, &p);
	/*if the retrieved property value is "stack", return*/
	if(strcmp(p,"stack")==0)
		return;

/*changing the old stack window to a master property*/
	Window old_stack=cursorless_wins[cursorless_length-2];
	XChangeProperty(screen->dpy, old_stack, status,utf8_string,8,
							PropModeReplace,(unsigned char*) "master",6);
/*the window that is pressed is now a stack window*/
	Window new_stack=event->subwindow;
	XChangeProperty(screen->dpy, new_stack, status,utf8_string,8,
							PropModeReplace,(unsigned char*) "stack",5);
/*configuring the attributes of the new master window*/
	XWindowChanges attr;
	attr.x=0;
	attr.y=(showbar?20:0);
	attr.width=screen->s_width/2;
	attr.height=(screen->s_height)-(showbar?20:0);
	XConfigureWindow(screen->dpy,old_stack,CWX | CWY | CWWidth | CWHeight,&attr);

	/*reorganize the list after the structure has been messed up*/
	organizeCursorLessList();

/*re-tile the stack windows*/
	tileStack();

}
void tileStack()
{
	if(cursorless_length<2)
		return;
/*observe that the next two lines do nothing unless tileStack is invoked by mapRequest()*/

	Window move_to_stack=cursorless_wins[cursorless_length-2];
	XChangeProperty(screen->dpy, move_to_stack, status, utf8_string, 8, PropModeReplace,
							(unsigned char *) "stack", 5);
	/*number of windows in stack*/
	int stack_num=cursorless_length-1;
	
	XWindowChanges attr;
	attr.width=screen->s_width/2;
	attr.height=(screen->s_height-(showbar?20:0))/stack_num;
	attr.x=screen->s_width/2;
	for(int i=0; i<stack_num;i++)
	{
		/*only the y attribute varies for stack windows*/
		attr.y=(stack_num-i-1)*attr.height+(showbar?20:0);		
		XConfigureWindow(screen->dpy,cursorless_wins[i], CWX | CWY | CWWidth | CWHeight, &attr);
	}
}
void removeFromCursorLessList(Window win)
{
	int i=0;
	/*solving for the index of the window in the list*/
	for(i;i<cursorless_length;i++)
	{
		if(cursorless_wins[i]==win)
			break;
	}
	/*since the entry at i is null now, move every entry after i one index forward*/
	for(i;i<cursorless_length-1;i++)
	{
		cursorless_wins[i]=cursorless_wins[i+1];
	}
	/*notice we never actually empty index i+1. Decrementing the cursorless_length variable is enough*/
	cursorless_length --;

} 
void removeFromCursorList(Window win)
{
	/*removeFromCursorList follows the same algorithm as removeFromCursorLessList*/
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
	cursor_length --;
}		

int wmError(Display *dpy, XErrorEvent *ev)
{
	fprintf(stderr, "another wm is already running");
	exit(1);
	return -1;
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
void xft_color_alloc(MyScreen *screen, XftColor *color, const char *colorName)
{
	if (!screen || !color || !colorName)
		return;
	/*XftColorAllocName requires a visual type and a colormap. The visual and colormap differs slightly. Visuals specify the supported depths, while colormap specifies the mapping of a monitor from pixels to RGB values*/
	if (!XftColorAllocName(screen->dpy, DefaultVisual(screen->dpy, screen->screen_num),
	                       screen->cmap,
	                       colorName, color))
		fprintf(stderr,"error, cannot allocate color '%s'", colorName);
}

static XftFont *xft_font_alloc(MyScreen *screen, const char *fontname)
{
	XftFont *font = NULL;
	/*the Xft library supports many ways of etablishing the FCPattern attribute of XftFont struct, among thes include passing a FCPattern struct or string. Here, however, we're choosing to pass a FontConfig string*/
	if (fontname && screen) {
			if (!(font = XftFontOpenName(screen->dpy, screen->screen_num, fontname))) {
			fprintf(stderr, "error with name '%s'\n", fontname);
			return NULL;
		}
	}
	return font;
}

int main()
{
	init();
	eventloop();
	return 0;
	
}

/*appearances*/
static const int showbar=1;/*show bar or not*/
static const int topbar=1;/*location of bar*/
static const int cursor=1;/*cursor style for the reparenting mode*/
static const char *g_font="monospace:size=10";
static const char *active_mode_col="#005577";/*color indicating the currently active bar mode*/
static const char *bar_col="#666666";/*color of bar*/
static const char *parent_col="#DCDCDC"; /*color of the parenting window*/
static const Pixmap background; /*background image of the cursor mode*/

/*key bindings*/
#define MODEMASK Mod1Mask
#define WINDOWMASK ShiftMask/*change focus&input focus with WINOWMASK+window number*/

static const int cursorless_mode_num = 1; /*switch to cursorless mode with MODEMASK+1*/
static const int cursor_mode_num = 2; /*switch to cursor mode with MODEMASK+2*/

static Key keys[]={
	{MODEMASK, XK_1, switchToCursorless},
	{MODEMASK, XK_2, switchToCursor},
	{WINDOWMASK, XK_a, switchToMaster},
	{WINDOWMASK, XK_d, switchToSlave},
	{WINDOWMASK, XK_c, destroyWin},
	{WINDOWMASK, XK_t, createTerm}
	
};

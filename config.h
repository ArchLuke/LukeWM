/*appearances*/
static const int showbar=1;/*show bar or not*/
static const int topbar=1;/*location of bar*/
static const int cursor=1;/*cursor style for the reparenting mode*/
static const char *font="monospace:size=10";
static const char *active_mode_col="#005577";/*color indicating the currently active bar mode*/
static const char *bar_col="#000000";/*color of bar*/
static const char parent_col="#ffffff"; /*color of the parenting window*/
static const Pixmap background; /*background image of the cursor mode*/

/*key bindings*/
#define MODEMASK Mod1Mask;
#define WINDOWMASK ShiftMask;/*change focus&input focus with WINOWMASK+window number*/

static const int cursorless_mode 1; /*switch to cursorless mode with MODEMASK+1*/
static const int cursor_mode 2; /*switch to cursor mode with MODEMASK+2*/

static const char master="A";/*moving a window to the master pane with WINDOWMASK+A*/
static const char slave="D";/*moving a window to the slave pane with WINDOWMASK+D*/
static const char destroy="C";/*destroying window with WINDOWMASK+C;*/
static const char new_term="T"i;/*launching a new xterm with WINDOWMASK+T*/

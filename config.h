/* appearance */
static const unsigned int borderpx  = 4;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const char col_gray1[]       = "#222222";
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#bbbbbb";
static const char col_gray4[]       = "#eeeeee";
static const char col_cyan[]        = "#005577";
static const char *colors[][3]      = {
	/*               fg         bg         border   */
	[SchemeNorm] = { col_gray3, col_gray1, col_gray2 },
	[SchemeSel]  = { col_gray4, col_cyan,  col_cyan  },
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4" };

/* layout(s) */
static const float mfact     = 0.6; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ tile },    /* first entry is default */
	{ monocle },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

static Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,           XK_e,           focusstack,     {.i = +1 } },
	{ MODKEY,           XK_o,           focusstack,     {.i = -1 } },
	{ MODKEY,           XK_i,           setmfact,       {.f = -0.1} },
	{ MODKEY,           XK_a,           setmfact,       {.f = +0.1} },
	{ MODKEY,           XK_Return,      zoom,           {0} },
	{ MODKEY,           XK_Tab,         view,           {0} },
	{ MODKEY|ShiftMask, XK_q,           killclient,     {0} },
	{ MODKEY,           XK_e|ShiftMask, setlayout,      {.v = &layouts[0]} },
	{ MODKEY,           XK_o|ShiftMask, setlayout,      {.v = &layouts[1]} },
	{ MODKEY|ShiftMask, XK_f,           togglefloating, {0} },
	{ MODKEY,           XK_comma,       focusmon,       {.i = -1 } },
	{ MODKEY,           XK_period,      focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask, XK_comma,       tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask, XK_period,      tagmon,         {.i = +1 } },
	{ MODKEY|ShiftMask, XK_q|ShiftMask, quit,           {0} },
	TAGKEYS(            XK_w,         0)
	TAGKEYS(            XK_u,         1)
	TAGKEYS(            XK_v,         2)
	TAGKEYS(            XK_semicolon, 3)
};

/* appearance */
static const unsigned int borderpx  = 4;        /* border pixel of windows */
static const char col_gray1[]       = "#000000";
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#bbbbbb";
static const char col_gray4[]       = "#eeeeee";
static const char col_cyan[]        = "#00aaff";
static const char *colors[][3]      = {
	/*               fg         bg         border   */
	[SchemeNorm] = { col_gray3, col_gray1, col_gray2 },
	[SchemeSel]  = { col_gray4, col_cyan,  col_cyan  },
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4" };

/* layout(s) */
static const float mfact     = 0.6; /* factor of master area size [0.05..0.95] */

static const Layout layouts[] = {
	{ tile },
	{ bstackhoriz },
	{ monocle },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

static Key keys[] = {
	/* modifier           key           function        argument */
	{ MODKEY,             XK_e,          focusstack,     {.i = +1 } },
	{ MODKEY,             XK_o,          focusstack,     {.i = -1 } },
	{ MODKEY|ControlMask, XK_e,          cyclelayout,    {.i = +1 } },
	{ MODKEY|ControlMask, XK_o,          cyclelayout,    {.i = -1 } },
	{ MODKEY,             XK_i,          focusmon,       {.i = -1 } },
	{ MODKEY,             XK_a,          focusmon,       {.i = +1 } },
	{ MODKEY|ControlMask, XK_i,          tagmon,         {.i = -1 } },
	{ MODKEY|ControlMask, XK_a,          tagmon,         {.i = +1 } },
	{ MODKEY,             XK_y,          setmfact,       {.f = +0.1} },
	{ MODKEY|ShiftMask,   XK_y,          setmfact,       {.f = -0.1} },
	{ MODKEY,             XK_apostrophe, zoom,           {0} },
	{ MODKEY,             XK_Tab,        view,           {0} },
	{ MODKEY,             XK_q,          killclient,     {0} },
	{ MODKEY|ShiftMask,   XK_f,          togglefloating, {0} },
	{ MODKEY,             XK_comma,      focusmon,       {.i = -1 } },
	{ MODKEY,             XK_period,     focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,   XK_q,          quit,           {0} },
	TAGKEYS(              XK_w,          0)
	TAGKEYS(              XK_u,          1)
	TAGKEYS(              XK_v,          2)
	TAGKEYS(              XK_semicolon,  3)
};

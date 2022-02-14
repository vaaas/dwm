#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.i = TAG}, }, \
	{ MODKEY|ControlMask,           KEY,      tag,            {.i = TAG}, },

static Key keys[] = {
	/* modifier           key            function        argument */
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
	TAGKEYS(              XK_w,          0)
	TAGKEYS(              XK_u,          1)
	TAGKEYS(              XK_v,          2)
	TAGKEYS(              XK_semicolon,  3)
};

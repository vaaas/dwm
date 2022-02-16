#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/keysym.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define INTERSECT(x,y,w,h,m) (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
							 * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C) (C->tag == C->mon->tag)
#define LENGTH(X) (sizeof X / sizeof X[0])
#define WIDTH(X) ((X)->w + 2 * borderpx)
#define HEIGHT(X) ((X)->h + 2 * borderpx)
#define LASTS(X) (X[strlen(X)-1])
#define LASTA(X) (X[sizeof X / sizeof X[0] - 1])
#define FOREACH(X, XS) for (X = XS; X; X = X->next)
#define FOREACHTILE(C, M) for (C = nexttiled(M->clients); C; C = nexttiled(C->next))
#define FIND(X, COND) while(X) { if (COND) break; else X = X->next; }
#define dwmfifo "/tmp/dwm.fifo"

void die(const char *msg) {
	fputs(msg, stderr);
	if (msg[0] && LASTS(msg) == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else fputc('\n', stderr);
	exit(1);
}

void *ecalloc(size_t length, size_t size) {
	void *p = calloc(length, size);
	if (!p) die("calloc:");
	return p;
}

enum EMWHAtom {
	NetSupported,
	NetWMName,
	NetWMState,
	NetWMCheck,
	NetWMFullscreen,
	NetActiveWindow,
	NetWMWindowType,
	NetWMWindowTypeDialog,
	NetClientList,
	NetLast
};

enum DefaultAtom {
	WMProtocols,
	WMDelete,
	WMState,
	WMTakeFocus,
	WMLast
};

typedef union {
	int i;
	float f;
} Arg;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	unsigned char tag;
	bool isfixed, isfloating, neverfocus, oldstate, isfullscreen;
	Client *next;
	Client *snext;
	Monitor *mon;
	Window win;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef void (*Layout)(Monitor *m);

struct Monitor {
	float mfact;
	int mx, my, mw, mh; // screen size
	int wx, wy, ww, wh; // window area
	unsigned char tag;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Layout layout;
};

enum ResourceType {
	INTEGER,
	FLOAT,
	HEX
};

// function declarations
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void cyclelayout(char x);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void dispatchcmd(void);
static void enternotify(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(char x);
static void focusstack(char x);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static void killclient();
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static Client *nexttiled(Client *c);
static void pop(Client *);
static void propertynotify(XEvent *e);
static Monitor *recttomon(int x, int y, int w, int h);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void restack(Monitor *m);
static void run(void);
static void scan(void);
static int sendevent(Client *c, Atom proto);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setlayout(Layout layout);
static void setmfact(float x);
static void setup(void);
static void showhide(Client *c);
static void sigchld(int unused);
static void tag(unsigned char x);
static void tagmon(char x);
static void togglefloating();
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updateclientlist(void);
static int updategeom(void);
static void updatesizehints(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void view(unsigned char x);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom();
static void load_xresources(Display *dpy);
static void resource_load(XrmDatabase db, char *name, enum ResourceType rtype, void *dst);
static float clamp(float x, float l, float h);
static unsigned int tile_count(Monitor *m);
static void monocle(Monitor *m);
static void tile(Monitor *m);
static void vstack (Monitor *m);
static void bstackhoriz(Monitor *m);
static Bool evpredicate();

static int screen;
static int sw, sh;			/* X display screen geometry width, height */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static void (*handler[LASTEvent]) (XEvent *) = {
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[FocusIn] = focusin,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[PropertyNotify] = propertynotify,
	[UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast];
static bool running = true;
static Display *dpy;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;

Layout layouts[] = { tile, monocle, };
unsigned char borderpx = 10;
unsigned char workspaces = 4;
unsigned char bh = 0;
unsigned long col_sel = 0x0000FF;
unsigned long col_norm = 0x000000;
float mfact = 0.6;
static int fifofd;

float clamp(float x, float l, float h) {
	return x < l ? l : x > h ? h : x;
}

unsigned int tile_count(Monitor *m) {
	 unsigned char n = 0;
	 Client *c;
	 FOREACHTILE(c, m) n++;
	 return n;
}

int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact) {
	int baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * borderpx < 0)
			*x = 0;
		if (*y + *h + 2 * borderpx < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * borderpx <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * borderpx <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (c->isfloating || !c->mon->layout) {
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void arrange(Monitor *m) {
	if (m) showhide(m->stack);
	else FOREACH(m, mons) { showhide(m->stack); }
	if (m) {
		arrangemon(m);
		restack(m);
	} else FOREACH(m, mons) { arrangemon(m); }
}

void arrangemon(Monitor *m) {
	if (m->layout)
		m->layout(m);
}

void attach(Client *c) {
	c->next = c->mon->clients;
	c->mon->clients = c;
}

void attachstack(Client *c) {
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void checkotherwm(void) {
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void cleanup(void) {
	Monitor *m;

	view(0);
	selmon->layout = NULL;
	FOREACH(m, mons)
		while (m->stack)
			unmanage(m->stack, 0);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);
	XDestroyWindow(dpy, wmcheckwin);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	close(fifofd);
	remove(dwmfifo);
}

void cleanupmon(Monitor *mon) {
	Monitor *m;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	free(mon);
}

void clientmessage(XEvent *e) {
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen] || cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(c,
				(cme->data.l[0] == 1 /* _NET_WM_STATE_ADD */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */
					&& !c->isfullscreen)));
	}
}

void configure(Client *c) {
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->isfullscreen ? 0 : borderpx;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void configurenotify(XEvent *e) {
	Monitor *m;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;

	/* TODO: updategeom handling sucks, needs to be simplified */
	if (ev->window == root) {
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
		if (updategeom() || dirty) {
			FOREACH(m, mons) {
				FOREACH(c, m->clients)
					if (c->isfullscreen)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
			}
			focus(NULL);
			arrange(NULL);
		}
	}
}

void configurerequest(XEvent *e) {
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (c->isfloating || !selmon->layout) {
			m = c->mon;
			if (ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if (ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

Monitor *createmon(void) {
	Monitor *m;
	m = ecalloc(1, sizeof(Monitor));
	m->tag = 0;
	m->mfact = mfact;
	m->layout = layouts[0];
	return m;
}

void cyclelayout(char x) {
	unsigned char i;
	for(i = 0; layouts[i] != selmon->layout; i++);
	if(x > 0) { // next layout
		if(layouts[i] && i < LENGTH(layouts) - 1)
			setlayout(layouts[i+1]);
		else
			setlayout(layouts[0]);
	} else { // previous layout
		if(i != 0 && layouts[i-1])
			setlayout(layouts[i-1]);
		else
			setlayout(LASTA(layouts));
	}
}

void destroynotify(XEvent *e) {
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);
}

void detach(Client *c) {
	Client **tc;

	for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void detachstack(Client *c) {
	Client **tc, *t;

	for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if (c == c->mon->sel) {
		for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
}

Monitor *dirtomon(int dir) { // finds the next or previous monitor
	Monitor *m = NULL;
	if (dir > 0) // find next monitor
		m = selmon->next ? selmon->next : mons;
	else { // find previous monitor
		m = mons;
		if (selmon == mons) // find the very last monitor
			FIND(m, m->next == NULL)
		else // find the monitor before the current monitor
			FIND(m, m->next == selmon)
	}
	return m;
}

void enternotify(XEvent *e) {
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
	} else if (!c || c == selmon->sel)
		return;
	focus(c);
}

void focus(Client *c) {
	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		detachstack(c);
		attachstack(c);
		XSetWindowBorder(dpy, c->win, col_sel);
		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
}

/* there are some broken focus acquiring clients needing extra handling */
void focusin(XEvent *e) {
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

void focusmon(char x) {
	Monitor *m;

	if (!mons->next)
		return;
	if ((m = dirtomon(x)) == selmon)
		return;
	unfocus(selmon->sel, 0);
	selmon = m;
	focus(NULL);
}

void focusstack(char x) {
	Client *c = NULL, *i;

	if (!selmon->sel || selmon->sel->isfullscreen)
		return;
	if (x > 0) {
		for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
		if (!c)
			for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
	} else {
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i))
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i))
					c = i;
	}
	if (c) {
		focus(c);
		restack(selmon);
	}
}

Atom getatomprop(Client *c, Atom prop) {
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		XFree(p);
	}
	return atom;
}

int getrootptr(int *x, int *y) {
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long getstate(Window w) {
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

static int isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info) {
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org && unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}

void killclient() {
	if (!selmon->sel)
		return;
	if (!sendevent(selmon->sel, wmatom[WMDelete])) {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void manage(Window w, XWindowAttributes *wa) {
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;

	c = ecalloc(1, sizeof(Client));
	c->win = w;
	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;

	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tag = t->tag;
	} else {
		c->mon = selmon;
		c->isfloating = 0;
		c->tag = c->mon->tag;
	}

	if (c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
		c->x = c->mon->mx + c->mon->mw - WIDTH(c);
	if (c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
		c->y = c->mon->my + c->mon->mh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->mx);
	/* only fix client y-offset, if the client center might cover the bar */
	c->y = MAX(c->y, ((bh == c->mon->my) && (c->x + (c->w / 2) >= c->mon->wx) && (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? bh : c->mon->my);

	wc.border_width = borderpx;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, col_norm);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	if (!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	if (c->isfloating)
		XRaiseWindow(dpy, c->win);
	attach(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	if (c->mon == selmon)
		unfocus(selmon->sel, 0);
	c->mon->sel = c;
	arrange(c->mon);
	XMapWindow(dpy, c->win);
	focus(NULL);
}

void mappingnotify(XEvent *e) {
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
}

void maprequest(XEvent *e) {
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	if (!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	if (wa.override_redirect)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

Client *nexttiled(Client *c) {
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

void pop(Client *c) {
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

void propertynotify(XEvent *e) {
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
			default: break;
			case XA_WM_TRANSIENT_FOR:
				if (!c->isfloating &&
					(XGetTransientForHint(dpy, c->win, &trans)) &&
					(c->isfloating = (wintoclient(trans)) != NULL))
					arrange(c->mon);
				break;
			case XA_WM_NORMAL_HINTS:
				updatesizehints(c);
				break;
			case XA_WM_HINTS:
				updatewmhints(c);
				break;
		}
		if (ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

Monitor *recttomon(int x, int y, int w, int h) {
	Monitor *m, *r = selmon;
	int a, area = 0;

	FOREACH(m, mons)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

void resize(Client *c, int x, int y, int w, int h, int interact) {
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void resizeclient(Client *c, int x, int y, int w, int h) {
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->isfullscreen ? 0 : borderpx;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

void restack(Monitor *m) {
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	if (!m->sel)
		return;
	if (m->sel->isfloating || !m->layout)
		XRaiseWindow(dpy, m->sel->win);
	if (m->layout) {
		wc.stack_mode = Below;
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void run(void) {
	XEvent ev;
	fd_set rfds;
	int n;
	int dpyfd, maxfd;
	XSync(dpy, False);
	dpyfd = ConnectionNumber(dpy);
	maxfd = MAX(dpyfd, fifofd) + 1;
	while (running) {
		FD_ZERO(&rfds);
		FD_SET(fifofd, &rfds);
		FD_SET(dpyfd, &rfds);
		n = select(maxfd, &rfds, NULL, NULL, NULL);
		if (n > 0) {
			if (FD_ISSET(fifofd, &rfds))
				dispatchcmd();
			if (FD_ISSET(dpyfd, &rfds))
				while (XCheckIfEvent(dpy, &ev, evpredicate, NULL))
					if (handler[ev.type])
						handler[ev.type](&ev); // call handler
		}
	}
}

void scan(void) {
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa) || wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1) && (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
}

void sendmon(Client *c, Monitor *m) {
	if (c->mon == m)
		return;
	unfocus(c, 1);
	detach(c);
	detachstack(c);
	c->mon = m;
	c->tag = m->tag; // assign tags of target monitor
	attach(c);
	attachstack(c);
	focus(NULL);
	arrange(NULL);
}

void setclientstate(Client *c, long state) {
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

int sendevent(Client *c, Atom proto) {
	int n;
	Atom *protocols;
	int exists = 0;
	XEvent ev;

	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}
	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	}
	return exists;
}

void setfocus(Client *c) {
	if (!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
	}
	sendevent(c, wmatom[WMTakeFocus]);
}

void setfullscreen(Client *c, int fullscreen) {
	if (fullscreen && !c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;
		c->oldstate = c->isfloating;
		c->isfloating = 1;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
	} else if (!fullscreen && c->isfullscreen){
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		c->isfloating = c->oldstate;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		arrange(c->mon);
	}
}

void setlayout(Layout layout) {
	if (layout) selmon->layout = layout;
	if (selmon->sel) arrange(selmon);
}

void setmfact(float x) {
	if (!x || !selmon->layout) return;
	selmon->mfact = clamp(x + selmon->mfact, 0.1, 0.9);
	arrange(selmon);
}

void setup(void) {
	XSetWindowAttributes wa;
	Atom utf8string;

	XrmInitialize();
	load_xresources(dpy);

	// clean up any zombies immediately
	sigchld(0);

	// init screen
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	updategeom();
	// init atoms
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	// supporting window for NetWMCheck
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "dwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	// EWMH support per view
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select events */
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	focus(NULL);

	mkfifo(dwmfifo, 0700);
	fifofd = open(dwmfifo, O_RDWR | O_NONBLOCK);
	if (fifofd < 0)
		die("Failed to open() DWM fifo" dwmfifo);
}

void showhide(Client *c) {
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		// show clients top down
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((!c->mon->layout || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		// hide clients bottom up
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void sigchld(int unused) {
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		die("can't install SIGCHLD handler:");
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void tag(unsigned char x) {
	if (selmon->sel) {
		selmon->sel->tag = x;
		focus(NULL);
		arrange(selmon);
	}
}

void tagmon(char x) {
	if (selmon->sel && mons->next)
		sendmon(selmon->sel, dirtomon(x));
}

void togglefloating() {
	if (!selmon->sel)
		return;
	if (selmon->sel->isfullscreen) // no support for fullscreen windows
		return;
	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	if (selmon->sel->isfloating)
		resize(selmon->sel, selmon->sel->x, selmon->sel->y,
			selmon->sel->w, selmon->sel->h, 0);
	arrange(selmon);
}

void unfocus(Client *c, int setfocus) {
	if (!c)
		return;
	XSetWindowBorder(dpy, c->win, col_norm);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

void unmanage(Client *c, int destroyed) {
	Monitor *m = c->mon;
	XWindowChanges wc;

	detach(c);
	detachstack(c);
	if (!destroyed) {
		wc.border_width = borderpx;
		XGrabServer(dpy); // avoid race conditions
		XSetErrorHandler(xerrordummy);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); // restore border
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);
	focus(NULL);
	updateclientlist();
	arrange(m);
}

void unmapnotify(XEvent *e) {
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
	}
}

void updateclientlist() {
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	FOREACH(m, mons)
		FOREACH(c, m->clients)
			XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend, (unsigned char *) &(c->win), 1);
}

int updategeom(void) {
	int dirty = 0;

	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++);
		// only consider unique geometries as separate screens
		unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;
		if (n <= nn) { // new monitors available
			for (i = 0; i < (nn - n); i++) {
				for (m = mons; m && m->next; m = m->next);
				if (m)
					m->next = createmon();
				else
					mons = createmon();
			}
			for (i = 0, m = mons; i < nn && m; m = m->next, i++)
				if (i >= n || unique[i].x_org != m->mx || unique[i].y_org != m->my || unique[i].width != m->mw || unique[i].height != m->mh)
				{
					dirty = 1;
					m->mx = m->wx = unique[i].x_org;
					m->my = m->wy = unique[i].y_org;
					m->mw = m->ww = unique[i].width;
					m->mh = m->wh = unique[i].height;
				}
		} else { // less monitors available nn < n
			for (i = nn; i < n; i++) {
				for (m = mons; m && m->next; m = m->next);
				while ((c = m->clients)) {
					dirty = 1;
					m->clients = c->next;
					detachstack(c);
					c->mon = mons;
					attach(c);
					attachstack(c);
				}
				if (m == selmon)
					selmon = mons;
				cleanupmon(m);
			}
		}
		free(unique);
	} else {
		if (!mons)
			mons = createmon();
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

void updatesizehints(Client *c) {
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		// size is uninitialized, ensure that size.flags aren't used
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
}

void updatewindowtype(Client *c) {
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
}

void updatewmhints(Client *c) {
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		}
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
	}
}

void view(unsigned char x) {
	if (x == selmon->tag) return;
	selmon->tag = x;
	focus(NULL);
	arrange(selmon);
}

Client *wintoclient(Window w) {
	Client *c;
	Monitor *m;

	FOREACH(m, mons) {
		FOREACH(c, m->clients) {
			if (c->win == w)
				return c;
		}
	}
	return NULL;
}

Monitor *wintomon(Window w) {
	int x, y;
	Client *c;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	else if ((c = wintoclient(w)))
		return c->mon;
	else
		return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlib's
 * default error handler, which may call exit. */
int xerror(Display *dpy, XErrorEvent *ee) {
	if (ee->error_code == BadWindow
		|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
		|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
		|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
		|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
		|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
		|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
		|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr,
		"dwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); // may call exit
}

int xerrordummy(Display *dpy, XErrorEvent *ee) { return 0; }

// Startup Error handler to check if another window manager is already running.
int xerrorstart(Display *dpy, XErrorEvent *ee) {
	die("dwm: another window manager is already running");
	return -1;
}

void zoom() {
	Client *c = selmon->sel;

	if (!selmon->layout || (selmon->sel && selmon->sel->isfloating))
		return;
	if (c == nexttiled(selmon->clients))
		if (!c || !(c = nexttiled(c->next)))
			return;
	pop(c);
}

// layouts
void monocle(Monitor *m) {
	Client *c;
	FOREACHTILE(c, m)
		resize(c, m->wx, m->wy, m->ww - borderpx*2, m->wh - borderpx*2, 0);
}

void tile(Monitor *m) {
	if (m->mw > m->mh) vstack(m);
	else bstackhoriz(m);
}

void vstack(Monitor *m) {
	unsigned int i, h, mw, my, ty;
	Client *c;

	unsigned int n = tile_count(m);
	switch(n) {
		case 0:
			return;
		case 1:
			monocle(m);
			return;
		default:
			mw = m->ww * m->mfact;
			break;
	}
	for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		if (i < 1) {
			h = (m->wh - my) / (MIN(n, 1) - i);
			resize(c, m->wx, m->wy + my, mw - (2*borderpx), h - (2*borderpx), 0);
			my += HEIGHT(c);
		} else {
			h = (m->wh - ty) / (n - i);
			resize(c, m->wx + mw, m->wy + ty, m->ww - mw - (2*borderpx), h - (2*borderpx), 0);
			ty += HEIGHT(c);
		}
}

void bstackhoriz(Monitor *m) {
	int w, mh, mx, tx, ty, th;
	unsigned int i;
	Client *c;
	unsigned int n = tile_count(m);

	switch(n) {
		case 0:
			return;
		case 1:
			monocle(m);
			return;
		default:
			mh = m->mfact * m->wh;
			th = (m->wh - mh) / (n - 1);
			ty = m->wy + mh;
	}
	for (i = mx = 0, tx = m->wx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++) {
		if (i < 1) {
			w = (m->ww - mx) / (MIN(n, 1) - i);
			resize(c, m->wx + mx, m->wy, w - (2 * borderpx), mh - (2 * borderpx), 0);
			mx += WIDTH(c);
		} else {
			resize(c, tx, ty, m->ww - (2 * borderpx), th - (2 * borderpx), 0);
			if (th != m->wh)
				ty += HEIGHT(c);
		}
	}
}

void load_xresources(Display *dpy) {
	char *resm;
	XrmDatabase db;

	resm = XResourceManagerString(dpy);
	if (!resm) return;
	db = XrmGetStringDatabase(resm);
	resource_load(db, "borderpx", INTEGER, &borderpx);
	resource_load(db, "workspaces", INTEGER, &workspaces);
	resource_load(db, "bh", INTEGER, &bh);
	resource_load(db, "col_sel", HEX, &col_sel);
	resource_load(db, "col_norm", HEX, &col_norm);
	resource_load(db, "mfact", FLOAT, &mfact);
}

void resource_load(XrmDatabase db, char *name, enum ResourceType rtype, void *dst) {
	char fullname[256];
	char *type;
	XrmValue ret;
	unsigned long *ul = dst;
	float *fl = dst;
	unsigned char *uc = dst;

	snprintf(fullname, sizeof(fullname), "%s.%s", "dwm", name);
	fullname[sizeof(fullname) - 1] = '\0';

	XrmGetResource(db, fullname, "*", &type, &ret);
	if (!(ret.addr == NULL || strncmp("String", type, 64))) {
		switch (rtype) {
		case INTEGER:
			*uc = (unsigned char) strtoul(ret.addr, NULL, 10);
			break;
		case FLOAT:
			*fl = strtof(ret.addr, NULL);
			break;
		case HEX:
			*ul = strtoul(ret.addr, NULL, 16);
			break;
		}
	}
}

Bool evpredicate() { return True; }

void dispatchcmd(void) {
	unsigned char c;

	read(fifofd, &c, 1);
	switch (c) {
		case 'a': view(0); break;
		case 'b': view(1); break;
		case 'c': view(2); break;
		case 'd': view(3); break;

		case 'A': tag(0); break;
		case 'B': tag(1); break;
		case 'C': tag(2); break;
		case 'D': tag(3); break;

		case 'w': focusstack(+1); break;
		case 'W': focusstack(-1); break;

		case 'l': cyclelayout(+1); break;
		case 'L': cyclelayout(-1); break;

		case 'm': focusmon(+1); break;
		case 'M': focusmon(-1); break;

		case 't': tagmon(-1); break;
		case 'T': tagmon(+1); break;

		case 'r': setmfact(+0.1); break;
		case 'R': setmfact(-0.1); break;

		case 'z': zoom(); break;
		case 'q': killclient(); break;
		case 'f': togglefloating(); break;

		default: break;
	}
}

int main(int argc, char *argv[]) {
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-"VERSION);
	else if (argc != 1)
		die("usage: dwm [-v]");
	else if (!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display");
	checkotherwm();
	setup();
	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}

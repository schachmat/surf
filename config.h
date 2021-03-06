/* modifier 0 means no modifier */
static char *useragent      = "Mozilla/5.0 (Windows NT 6.1; rv:31.0) "
                              "Gecko/20100101 Firefox/31.0";
static char *scriptfile     = "~/.surf/script.js";
static char *historyfile    = "~/.surf/history";
static char *styledir       = "~/.surf/styles/";
static char *cachefolder    = "~/.surf/cache/";

static Bool kioskmode       = FALSE; /* Ignore shortcuts */
static Bool showindicators  = TRUE;  /* Show indicators in window title */
static Bool zoomto96dpi     = TRUE;  /* Zoom pages to always emulate 96dpi */
static Bool runinfullscreen = FALSE; /* Run in fullscreen mode by default */
/* Refuse untrusted SSL connections */
#define HOMEPAGE "https://startpage.com/"

static guint defaultfontsize = 12;   /* Default font size */
static gfloat zoomlevel = 1.0;       /* Default zoom level */

/* Soup default features */
static char *cookiefile     = "~/.surf/cookies.txt";
static char *cookiepolicies = "Aa@"; /* A: accept all; a: accept nothing,
                                      * @: accept all except third party */
static char *cafile         = "/etc/ssl/certs/ca-certificates.crt";
static Bool strictssl       = FALSE; /* Refuse untrusted SSL connections */
static time_t sessiontime   = 3600;

/* Webkit default features */
static Bool enablescrollbars      = TRUE;
static Bool enablespatialbrowsing = TRUE;
static Bool enablediskcache       = TRUE;
static int diskcachebytes         = 5 * 1024 * 1024;
static Bool enableplugins         = TRUE;
static Bool enablescripts         = TRUE;
static Bool enableinspector       = TRUE;
static Bool enablestyle           = TRUE;
static Bool loadimages            = TRUE;
static Bool hidebackground        = FALSE;
static Bool allowgeolocation      = TRUE;

#define SETURI { \
	.v = (char *[]){ "/bin/sh", "-c", \
	     "prop=\"`xprop -id $0 _SURF_URI" \
	     " | sed \"s/^_SURF_URI(STRING) = \\(\\\\\"\\?\\)\\(.*\\)\\1$/\\2/\" " \
	     " | tac - \"${HOME}/.surf/history\"" \
	     " | awk '!x[$0]++'" \
	     " | dmenu -i -l 10`\"" \
	     " && xprop -id $0 -f _SURF_GO 8s -set _SURF_GO \"$prop\"", \
	     winid, NULL \
	} \
}

#define SETSEARCH { \
	.v = (char *[]){ "/bin/sh", "-c", \
	     "prop=\"`xprop -id $0 _SURF_FIND" \
	     " | sed \"s/^_SURF_FIND(STRING) = \\(\\\\\"\\?\\)\\(.*\\)\\1$/\\2/\" " \
	     " | tac - \"${HOME}/.surf/searches\"" \
	     " | awk '!x[$0]++'" \
	     " | xargs -0 printf %b" \
	     " | dmenu -i -l 10`\"" \
	     " && xprop -id $0 -f _SURF_FIND 8s -set _SURF_FIND \"$prop\"" \
	     " && echo \"$prop\" >> \"${HOME}/.surf/searches\"", \
	     winid, NULL \
	} \
}

#define SELNAV { \
	.v = (char *[]){ "/bin/sh", "-c", \
	     "prop=\"`xprop -id $0 _SURF_HIST" \
	     " | sed -e 's/^.[^\"]*\"//' -e 's/\"$//' -e 's/\\\\\\n/\\n/g'" \
	     " | dmenu -i -l 10`\"" \
	     " && xprop -id $0 -f _SURF_NAV 8s -set _SURF_NAV \"$prop\"", \
	     winid, NULL \
	} \
}

/* DOWNLOAD(URI, referer) */
#define DOWNLOAD(d, r) { \
	.v = (char *[]){ "/bin/sh", "-c", \
	     "st -e /bin/sh -c \"cd ${HOME}/downloads" \
	     " && curl -k -L -J -O --user-agent '$1' --referer '$2' -b $3 -c $3 '$0';" \
	     " sleep 5;\"", \
	     d, useragent, r, cookiefile, NULL \
	} \
}

/* PLUMB(URI) */
/* This called when some URI which does not begin with "about:",
 * "http://" or "https://" should be opened.
 */
#define PLUMB(u) {\
	.v = (char *[]){ "/bin/sh", "-c", \
	     "xdg-open \"$0\"", u, NULL \
	} \
}

/* styles */
/*
 * The iteration will stop at the first match, beginning at the beginning of
 * the list.
 */
static SiteStyle styles[] = {
	/* regexp                                      file in $styledir */
	{ "^https?://([a-z0-9-]+\\.)*startpage\\.com", "startpage.css" },
	{ "^https?://([a-z0-9-]+\\.)*wikipedia\\.org", "wiki.css" },
	{ "^https?://github\\.com",                    "github.css" },
	{ "^https?://([a-z0-9-]+\\.)*youtube\\.com",   "youtube.css" },
	{ ".*",                                        "default.css" },
};

#define MODKEY GDK_CONTROL_MASK

// searchengines
static SearchEngine searchengines[] = {
	{ "s",        "https://startpage.com/do/search?q=%s" },
	{ "l",        "https://dict.leo.org/#/search=%s" },
	{ "w",        "https://en.wikipedia.org/wiki/%s" },
	{ "man",      "http://localhost:8626/%s" },
};

/* hotkeys */
/*
 * If you use anything else but MODKEY and GDK_SHIFT_MASK, don't forget to
 * edit the CLEANMASK() macro.
 */
static Key keys[] = {
	/* modifier             keyval      function    arg             Focus */
	{ MODKEY|GDK_SHIFT_MASK,GDK_r,      reload,     { .b = TRUE } },
	{ MODKEY,               GDK_r,      reload,     { .b = FALSE } },
	{ MODKEY|GDK_SHIFT_MASK,GDK_p,      print,      { 0 } },

	{ MODKEY,               GDK_p,      clipboard,  { .b = TRUE } },
	{ MODKEY,               GDK_y,      clipboard,  { .b = FALSE } },

	{ MODKEY|GDK_SHIFT_MASK,GDK_j,      zoom,       { .i = -1 } },
	{ MODKEY|GDK_SHIFT_MASK,GDK_k,      zoom,       { .i = +1 } },
	{ MODKEY|GDK_SHIFT_MASK,GDK_q,      zoom,       { .i = 0  } },
	{ MODKEY,               GDK_minus,  zoom,       { .i = -1 } },
	{ MODKEY,               GDK_plus,   zoom,       { .i = +1 } },

	{ MODKEY,               GDK_l,      navigate,   { .i = +1 } },
	{ MODKEY,               GDK_h,      navigate,   { .i = -1 } },

	{ MODKEY,               GDK_j,      selhist,    SELNAV },
	{ MODKEY,               GDK_k,      scroll_v,   { .i = -1 } },
	{ MODKEY,               GDK_b,      scroll_v,   { .i = -10000 } },
	{ MODKEY,               GDK_space,  scroll_v,   { .i = +10000 } },
	{ MODKEY,               GDK_i,      scroll_h,   { .i = +1 } },
	{ MODKEY,               GDK_u,      scroll_h,   { .i = -1 } },

	{ 0,                    GDK_F11,    fullscreen, { 0 } },
	{ 0,                    GDK_Escape, stop,       { 0 } },
	{ MODKEY,               GDK_o,      source,     { 0 } },
	{ MODKEY|GDK_SHIFT_MASK,GDK_o,      inspector,  { 0 } },

	{ MODKEY,               GDK_g,      spawn,      SETURI },
	{ MODKEY,               GDK_f,      spawn,      SETSEARCH },
	{ MODKEY,               GDK_slash,  spawn,      SETSEARCH },

	{ MODKEY,               GDK_n,      find,       { .b = TRUE } },
	{ MODKEY|GDK_SHIFT_MASK,GDK_n,      find,       { .b = FALSE } },

	{ MODKEY|GDK_SHIFT_MASK,GDK_c,      toggle,     { .v = "enable-caret-browsing" } },
	{ MODKEY|GDK_SHIFT_MASK,GDK_i,      toggle,     { .v = "auto-load-images" } },
	{ MODKEY|GDK_SHIFT_MASK,GDK_s,      toggle,     { .v = "enable-scripts" } },
	{ MODKEY|GDK_SHIFT_MASK,GDK_v,      toggle,     { .v = "enable-plugins" } },
	{ MODKEY|GDK_SHIFT_MASK,GDK_a,      togglecookiepolicy, { 0 } },
	{ MODKEY|GDK_SHIFT_MASK,GDK_m,      togglestyle,{ 0 } },
	{ MODKEY|GDK_SHIFT_MASK,GDK_b,      togglescrollbars,{ 0 } },
	{ MODKEY|GDK_SHIFT_MASK,GDK_g,      togglegeolocation, { 0 } },
};

/* button definitions */
/* click can be ClkDoc, ClkLink, ClkImg, ClkMedia, ClkSel, ClkEdit, ClkAny */
static Button buttons[] = {
	/* click        event mask  button  function        argument */
	{ ClkLink,      0,          2,      linkopenembed,  { 0 } },
	{ ClkLink,      MODKEY,     2,      linkopen,       { 0 } },
	{ ClkLink,      MODKEY,     1,      linkopenembed,  { 0 } },
	{ ClkAny,       0,          8,      navigate,       { .i = -1 } },
	{ ClkAny,       0,          9,      navigate,       { .i = +1 } },
};

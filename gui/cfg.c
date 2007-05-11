#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "leitm.h"

#define DEMO_BIN	"./mlfc_unix"

void run_handler(Widget w, void *cdata, void *cbs);
void quit_handler(Widget w, void *cdata, void *cbs);

lmwidget *win;

int res[] = {0, 1, 0, 0, 0, 0};
char *res_str[] = {
	"800x600",
	"1024x768",
	"1280x960",
	"1280x1024",
	"1600x1200",
	0
};

int fullscreen = 1;
int music = 1;

char **foo_argv;
int foo_argc;

int main(int argc, char **argv)
{
	Display *dpy;
	Window xwin;
	Widget mw;
	char *title = "demo configurator";
	XTextProperty wname;
	int i;
	lmwidget *hbox, *vbox, *vbox2, *rbox, *bbox;

	foo_argv = malloc((argc + 2) * sizeof *foo_argv);
	memcpy(foo_argv, argv, (argc + 1) * sizeof *foo_argv);
	foo_argv[0] = DEMO_BIN;
	foo_argc = argc;

	if(!(win = lm_init_args(&argc, argv))) {
		return -1;
	}

	hbox = lm_hbox(win);
	vbox = lm_vbox(hbox);

	lm_label(vbox, "resolution");
	
	rbox = lm_radio_box(vbox);
	for(i=0; res_str[i]; i++) {
		lm_radio_link(rbox, res_str[i], res[i], &res[i]);
	}

	lm_separator(hbox, LM_VERTICAL);
	vbox2 = lm_vbox(hbox);

	lm_label(vbox2, "options");
	lm_check_link(vbox2, "fullscreen", fullscreen, &fullscreen);
	lm_check_link(vbox2, "music", music, &music);

	lm_separator(vbox2, LM_HORIZONTAL);

	lm_button_call(vbox2, "run demo", run_handler);
	lm_button_call(vbox2, "quit", quit_handler);

	lm_show(win);

	mw = lm_get_motif_widget(win);
	dpy = XtDisplay(mw);
	xwin = XtWindow(mw);
	XStringListToTextProperty((char**)&title, 1, &wname);
	XSetWMName(dpy, xwin, &wname);
	XFree(wname.value);

	lm_main_loop();
	return 0;
}

void run_handler(Widget w, void *cdata, void *cbs)
{
	FILE *fp;
	int i, ridx = 1;

	if(!(fp = fopen("3dengfx.conf", "w"))) {
		perror("failed to write config file");
		exit(1);
	}

	for(i=0; res_str[i]; i++) {
		if(res[i]) {
			ridx = i;
			break;
		}
	}

	fprintf(fp, "fullscreen = %s\n", fullscreen ? "true" : "false");
	fprintf(fp, "resolution = %s\n", res_str[ridx]);
	fprintf(fp, "bpp = dontcare\nzbuffer = dontcare\nstencil = dontcare\n");

	fclose(fp);

	if(!music) {
		foo_argv[foo_argc++] = "-m";
		foo_argv[foo_argc] = 0;
	}

	execv(DEMO_BIN, foo_argv);
	perror("failed to exec " DEMO_BIN);
	exit(1);
}

void quit_handler(Widget w, void *cdata, void *cbs)
{
	exit(0);
}

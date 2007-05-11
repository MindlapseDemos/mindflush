#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _SGIMOTIF
#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/TextF.h>
#include <Xm/Scale.h>
#include <Xm/ToggleB.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/MessageB.h>
#include <Xm/FileSB.h>
#include "leitm.h"

#define FLTSCALE_DECIMALS		2
#define FLTSCALE_MULTIPLIER		100

struct lmwidget {
	Widget w;
	enum link_type {LINK_NONE, LINK_INT, LINK_FLT, LINK_STR} ltype;
	union {
		int *ilink;
		float *flink;
		char *slink;
	} link;
	size_t slink_buf_size;

	struct lmwidget *next;
	struct lmwidget *children;
};

static void textfield_link_handler(Widget w, void *cdata, void *cbs);
static void scale_link_handler(Widget w, void *cdata, void *cbs);
static void toggle_link_handler(Widget w, void *cdata, void *cbs);

static XtAppContext app;


lmwidget *lm_create_widget(void)
{
	lmwidget *w = malloc(sizeof *w);
	if(w) {
		memset(w, 0, sizeof *w);
		w->next = malloc(sizeof *w);
		w->next->next = w->next->children = 0;
		
		w->children = malloc(sizeof *w);
		w->children->next = w->children->children = 0;
	}
	return w;
}

void lm_free_widget(lmwidget *w)
{
	lmwidget *iter = w->children->next;

	while(iter) {
		lmwidget *tmp = iter;
		iter = iter->next;
		lm_free_widget(tmp);
	}
}

Widget lm_get_motif_widget(lmwidget *w)
{
	if(w) return w->w;
	return 0;
}

lmwidget *lm_init(void)
{
	char *argv[] = {"./foo", 0};
	int argc = 1;

	return lm_init_args(&argc, argv);
}

lmwidget *lm_init_args(int *argc, char **argv)
{
	lmwidget *w;

	static char *res[] = {
		"*sgiMode: true",
		"*useSchemes: all",
		0
	};

	if((w = lm_create_widget())) {
		w->w = XtVaAppInitialize(&app, "leitmotif", NULL, 0, argc, argv, res, NULL);
		w->ltype = LINK_NONE;
	}
	return w;
}

void lm_show(lmwidget *w)
{
	XtRealizeWidget(w->w);
}

void lm_main_loop(void)
{
	XtAppMainLoop(app);
}

#define ADD_CHILD(p, c)	\
	c->next = p->children->next; \
	p->children->next = c;

/* packing */
lmwidget *lm_vbox(lmwidget *parent)
{
	lmwidget *w;

	if((w = lm_create_widget())) {
		w->w = XtVaCreateManagedWidget("rowcolumn", xmRowColumnWidgetClass, parent->w, NULL);
		w->ltype = LINK_NONE;
		ADD_CHILD(parent, w);
	}
	return w;
}

lmwidget *lm_hbox(lmwidget *parent)
{
	lmwidget *w;

	if((w = lm_create_widget())) {
		w->w = XtVaCreateManagedWidget("rowcolumn", xmRowColumnWidgetClass, parent->w, XmNorientation, XmHORIZONTAL, NULL);
		w->ltype = LINK_NONE;
		ADD_CHILD(parent, w);
	}
	return w;
}

/* label */
lmwidget *lm_label(lmwidget *parent, const char *text)
{
	lmwidget *w;

	if((w = lm_create_widget())) {
		w->w = XtVaCreateManagedWidget(text, xmLabelWidgetClass, parent->w, NULL);
		w->ltype = LINK_NONE;
		ADD_CHILD(parent, w);
	}
	return w;
}

/* textfield */
lmwidget *lm_textfield(lmwidget *parent, const char *text)
{
	lmwidget *w;

	if((w = lm_create_widget())) {
		w->w = XtVaCreateManagedWidget(text, xmTextFieldWidgetClass, parent->w, NULL);
		XmTextFieldSetString(w->w, (char*)text);
		w->ltype = LINK_NONE;
		ADD_CHILD(parent, w);
	}
	return w;
}

lmwidget *lm_textfield_slink(lmwidget *parent, const char *text, char *link, size_t sz)
{
	lmwidget *w;

	if((w = lm_textfield_call(parent, text, textfield_link_handler))) {
		w->ltype = LINK_STR;
		w->link.slink = link;
		w->slink_buf_size = sz;
		strncpy(w->link.slink, text, sz);
	}
	return w;
}

lmwidget *lm_textfield_ilink(lmwidget *parent, const char *text, int *link)
{
	lmwidget *w;

	if((w = lm_textfield_call(parent, text, textfield_link_handler))) {
		w->ltype = LINK_INT;
		w->link.ilink = link;
		*link = atoi(text);
	}
	return w;
}

lmwidget *lm_textfield_flink(lmwidget *parent, const char *text, float *link)
{
	lmwidget *w;

	if((w = lm_textfield_call(parent, text, textfield_link_handler))) {
		w->ltype = LINK_FLT;
		w->link.flink = link;
		*link = atof(text);
	}
	return w;
}


lmwidget *lm_textfield_call(lmwidget *parent, const char *text, lm_event_handler handler)
{
	lmwidget *w;

	if((w = lm_textfield(parent, text))) {
		XtAddCallback(w->w, XmNactivateCallback, handler, w);
		XtAddCallback(w->w, XmNlosingFocusCallback, handler, w);
	}
	return w;
}

void lm_textfield_set_text(lmwidget *w, const char *text)
{
	XmTextFieldSetString(w->w, (char*)text);
}

const char *lm_textfield_get_text(lmwidget *w)
{
	return XmTextFieldGetString(w->w);
}


/* button */
lmwidget *lm_button(lmwidget *parent, const char *text)
{
	lmwidget *w;

	if((w = lm_create_widget())) {
		w->w = XtVaCreateManagedWidget(text, xmPushButtonWidgetClass, parent->w, NULL);
		w->ltype = LINK_NONE;
		ADD_CHILD(parent, w);
	}
	return w;
}

lmwidget *lm_button_call(lmwidget *parent, const char *text, lm_event_handler handler)
{
	lmwidget *w;

	if((w = lm_button(parent, text))) {
		XtAddCallback(w->w, XmNactivateCallback, handler, w);
	}
	return w;
}

/* scales */
lmwidget *lm_intscale(lmwidget *parent, int orien, int show, int min, int max, int val)
{
	lmwidget *w;

	if((w = lm_create_widget())) {
		w->w = XtVaCreateManagedWidget("scale",
				xmScaleWidgetClass, parent->w,
				XmNminimum, min,
				XmNmaximum, max,
				XmNorientation, orien == LM_HORIZONTAL ? XmHORIZONTAL : XmVERTICAL,
				XmNdecimalPoints, 0,
				XmNshowValue, show,
				XmNvalue, val,
				NULL);
		w->ltype = LINK_NONE;
		ADD_CHILD(parent, w);
	}
	return w;
}

lmwidget *lm_intscale_link(lmwidget *parent, int orien, int show, int min, int max, int val, int *link)
{
	lmwidget *w;

	if((w = lm_intscale_call(parent, orien, show, min, max, val, scale_link_handler))) {
		w->ltype = LINK_INT;
		w->link.ilink = link;
	}
	return w;
}

lmwidget *lm_intscale_call(lmwidget *parent, int orien, int show, int min, int max, int val, lm_event_handler handler)
{
	lmwidget *w;

	if((w = lm_intscale(parent, orien, show, min, max, val))) {
		XtAddCallback(w->w, XmNvalueChangedCallback, handler, w);
	}
	return w;
}

void lm_intscale_set_value(lmwidget *w, int val)
{
	XmScaleSetValue(w->w, val);
}

int lm_intscale_get_value(lmwidget *w)
{
	int val;
	XmScaleGetValue(w->w, &val);
	return val;
}


lmwidget *lm_fltscale(lmwidget *parent, int orien, int show, float min, float max, float val)
{
	lmwidget *w;

	if((w = lm_create_widget())) {
		w->w = XtVaCreateManagedWidget("scale",
				xmScaleWidgetClass, parent->w,
				XmNminimum, (int)(min * FLTSCALE_MULTIPLIER),
				XmNmaximum, (int)(max * FLTSCALE_MULTIPLIER),
				XmNorientation, orien == LM_HORIZONTAL ? XmHORIZONTAL : XmVERTICAL,
				XmNdecimalPoints, FLTSCALE_DECIMALS,
				XmNshowValue, show,
				NULL);
		w->ltype = LINK_NONE;
		ADD_CHILD(parent, w);

		lm_fltscale_set_value(w, val);
	}
	return w;
}

lmwidget *lm_fltscale_link(lmwidget *parent, int orien, int show, float min, float max, float val, float *link)
{
	lmwidget *w;

	if((w = lm_fltscale_call(parent, orien, show, min, max, val, scale_link_handler))) {
		/*XtVaSetValues(w->w, XmNdragCallback, scale_link_handler);*/
		
		w->ltype = LINK_FLT;
		w->link.flink = link;
	}
	return w;
}

lmwidget *lm_fltscale_call(lmwidget *parent, int orien, int show, float min, float max, float val, lm_event_handler handler)
{
	lmwidget *w;

	if((w = lm_fltscale(parent, orien, show, min, max, val))) {
		XtAddCallback(w->w, XmNvalueChangedCallback, handler, w);
	}
	return w;
}

void lm_fltscale_set_value(lmwidget *w, float val)
{
	XmScaleSetValue(w->w, (int)(val * FLTSCALE_MULTIPLIER));
}

float lm_fltscale_get_value(lmwidget *w)
{
	int val;
	XmScaleGetValue(w->w, &val);
	return (float)val / (float)FLTSCALE_MULTIPLIER;
}


lmwidget *lm_progress(lmwidget *parent)
{
	lmwidget *w;

	if((w = lm_create_widget())) {
		w->w = XtVaCreateManagedWidget("scale",
				xmScaleWidgetClass, parent->w,
				XmNminimum, 0,
				XmNmaximum, (int)(1.0 * FLTSCALE_MULTIPLIER),
				XmNsensitive, 0,
				XmNorientation, XmHORIZONTAL,
				NULL);
		w->ltype = LINK_NONE;
		ADD_CHILD(parent, w);
	}
	return w;
}



/* radio buttons */

lmwidget *lm_radio_box(lmwidget *parent)
{
	lmwidget *w;

	if((w = lm_create_widget())) {
		w->w = XmCreateRadioBox(parent->w, "radio box", 0, 0);
		XtManageChild(w->w);
		ADD_CHILD(parent, w);
	}
	return w;
}

lmwidget *lm_radio(lmwidget *parent, const char *text, int on)
{
	lmwidget *w;

	if((w = lm_create_widget())) {
		w->w = XtVaCreateManagedWidget(text, xmToggleButtonWidgetClass, parent->w, XmNindicatorType, XmONE_OF_MANY, NULL);
		if(on) {
			XmToggleButtonSetState(w->w, True, False);
		}
		ADD_CHILD(parent, w);
	}
	return w;
}

lmwidget *lm_radio_link(lmwidget *parent, const char *text, int on, int *link)
{
	lmwidget *w;

	if((w = lm_radio_call(parent, text, on, toggle_link_handler))) {
		w->ltype = LINK_INT;
		w->link.ilink = link;
		*link = on;
	}
	return w;
}

lmwidget *lm_radio_call(lmwidget *parent, const char *text, int on, lm_event_handler handler)
{
	lmwidget *w;

	if((w = lm_radio(parent, text, on))) {
		XtAddCallback(w->w, XmNvalueChangedCallback, handler, w);
	}
	return w;
}

void lm_radio_set_state(lmwidget *w, int val)
{
	XmToggleButtonSetState(w->w, val, 1);
}

int lm_radio_get_state(lmwidget *w)
{
	return XmToggleButtonGetState(w->w);
}


/* check buttons */
lmwidget *lm_check(lmwidget *parent, const char *text, int on)
{
	lmwidget *w;

	if((w = lm_create_widget())) {
		w->w = XtVaCreateManagedWidget(text, xmToggleButtonWidgetClass, parent->w, NULL);
		if(on) {
			XmToggleButtonSetState(w->w, True, False);
		}
		ADD_CHILD(parent, w);
	}
	return w;
}

lmwidget *lm_check_link(lmwidget *parent, const char *text, int on, int *link)
{
	lmwidget *w;

	if((w = lm_check_call(parent, text, on, toggle_link_handler))) {
		w->ltype = LINK_INT;
		w->link.ilink = link;
		*link = on;
	}
	return w;
}

lmwidget *lm_check_call(lmwidget *parent, const char *text, int on, lm_event_handler handler)
{
	lmwidget *w;

	if((w = lm_check(parent, text, on))) {
		XtAddCallback(w->w, XmNvalueChangedCallback, handler, w);
	}
	return w;
}

void lm_check_set_state(lmwidget *w, int val)
{
	XmToggleButtonSetState(w->w, val, 1);
}

int lm_check_get_state(lmwidget *w)
{
	return XmToggleButtonGetState(w->w);
}


/* list */

lmwidget *lm_list(lmwidget *parent, int height, int msel)
{
	lmwidget *w;

	if((w = lm_create_widget())) {
		w->w = XtVaCreateManagedWidget("list", xmListWidgetClass, parent->w,
				XmNvisibleItemCount, height,
				XmNitemCount, 0,
				XmNscrollBarDisplayPolicy, XmAS_NEEDED,
				XmNselectionPolicy, msel ? XmMULTIPLE_SELECT : XmSINGLE_SELECT,
				NULL);
		ADD_CHILD(parent, w);
	}
	return w;
}

void lm_list_append(lmwidget *w, const char *str)
{
	XmString xm_str = XmStringCreateSimple((char*)str);
	XmListAddItem(w->w, xm_str, 0);
	XmStringFree(xm_str);
}

void lm_list_remove(lmwidget *w, const char *str)
{
	XmString xm_str = XmStringCreateSimple((char*)str);
	XmListDeleteItem(w->w, xm_str);
	XmStringFree(xm_str);
}


void lm_list_select(lmwidget *w, int idx)
{
	XmListSelectPos(w->w, idx, 1);
}

int lm_list_get_selected(lmwidget *w)
{
	int sel_num;
	int *sel_items;

	XmListGetSelectedPos(w->w, &sel_items, &sel_num);

	if(sel_num > 1) {
		fprintf(stderr, "ignoring more than the first selection\n");
	}
	return sel_items[0];
}


void lm_list_select_many(lmwidget *w, int *idx)
{
}

int *lm_list_get_selected_many(lmwidget *w)
{
	return 0;
}


lmwidget *lm_separator(lmwidget *parent, int orientation)
{
	lmwidget *w;
	if((w = lm_create_widget())) {
		w->w = XtVaCreateManagedWidget("sep", xmSeparatorWidgetClass, parent->w,
				XmNorientation, orientation == LM_HORIZONTAL ? XmHORIZONTAL : XmVERTICAL,
				NULL);
		ADD_CHILD(parent, w);
	}
	return w;
}

/* dialogs */
enum {
	DLG_INFO,
	DLG_WARN,
	DLG_ERR
};
static Widget (*dlg_func[3])(Widget, char*, Arg*, Cardinal) = {
	XmCreateInformationDialog,
	XmCreateWarningDialog,
	XmCreateErrorDialog
};
static char *dlg_title[] = {
	"info",
	"warning",
	"error"
};

static Widget create_dialog(Widget parent, const char *text, int type, int modal)
{
	Arg args[2];
	XmString xm_string;
	Widget dlg;

	xm_string = XmStringCreateSimple((char*)text);
	XtSetArg(args[0], XmNmessageString, xm_string);
	XtSetArg(args[1], XmNdialogStyle, modal ? XmDIALOG_APPLICATION_MODAL : XmDIALOG_MODELESS);
	dlg = (dlg_func[type])(parent, dlg_title[type], args, 2);
	XmStringFree(xm_string);

	return dlg;
}

void lm_message(lmwidget *parent, const char *text, int type)
{
	Widget dlg = create_dialog(parent->w, text, DLG_INFO, 0);

	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_CANCEL_BUTTON));
	XtManageChild(dlg);
}

void lm_message_call(lmwidget *parent, const char *text, int type, lm_event_handler handler)
{
	Widget dlg = create_dialog(parent->w, text, DLG_INFO, 0);
	XtAddCallback(dlg, XmNokCallback, handler, dlg);

	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_CANCEL_BUTTON));
	XtManageChild(dlg);
}

void lm_file(lmwidget *parent, const char *mask)
{
	Arg args[1];
	Widget dlg;

	XtSetArg(args[0], XmNdirMask, (char*)mask);
	dlg = XmCreateFileSelectionDialog(parent->w, "select", args, mask ? 1 : 0);
	XtUnmanageChild(XmFileSelectionBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));
	XtManageChild(dlg);
}

static void file_selector_ok_handler(Widget w, void *cdata, void *cbs)
{
	lm_event_handler handler = (lm_event_handler)cdata;
	handler(w, cdata, 0);
	XtUnmanageChild(w);
}

static void file_selector_cancel_handler(Widget w, void *cdata, void *cbs)
{
	XtUnmanageChild(w);
}

void lm_file_call(lmwidget *parent, const char *mask, lm_event_handler handler)
{
	Arg args[1];
	Widget dlg;

	XtSetArg(args[0], XmNdirMask, (char*)mask);
	dlg = XmCreateFileSelectionDialog(parent->w, "select", args, mask ? 1 : 0);
	
	XtAddCallback(dlg, XmNokCallback, file_selector_ok_handler, (void*)handler);
	XtAddCallback(dlg, XmNcancelCallback, file_selector_cancel_handler, 0);

	XtUnmanageChild(XmFileSelectionBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));
	XtManageChild(dlg);
}


/* ---------------- */
static void textfield_link_handler(Widget w, void *cdata, void *cbs)
{
	lmwidget *lmw = (lmwidget*)cdata;
	char *str = XmTextFieldGetString(w);
	if(!str) {
		fprintf(stderr, "textfield_link_handler failed, XmTextFieldGetString returned 0\n");
		return;
	}

	switch(lmw->ltype) {
	case LINK_STR:
		strncpy(lmw->link.slink, str, lmw->slink_buf_size);
		break;

	case LINK_INT:
		*lmw->link.ilink = atoi(str);
		break;

	case LINK_FLT:
		*lmw->link.flink = atof(str);
		break;

	default:
		break;
	}
}

static void scale_link_handler(Widget w, void *cdata, void *cbs)
{
	lmwidget *lmw = (lmwidget*)cdata;
	int val = ((XmScaleCallbackStruct*)cbs)->value;
	
	switch(lmw->ltype) {
	case LINK_INT:
		*lmw->link.ilink = val;
		break;

	case LINK_FLT:
		*lmw->link.flink = (float)val / (float)FLTSCALE_MULTIPLIER;
		break;

	case LINK_STR:
		snprintf(lmw->link.slink, lmw->slink_buf_size, "%d", val);
		break;

	default:
		break;
	}
}

static void toggle_link_handler(Widget w, void *cdata, void *cbs)
{
	lmwidget *lmw = (lmwidget*)cdata;
	int val = ((XmToggleButtonCallbackStruct*)cbs)->set;

	if(lmw->ltype == LINK_INT) {
		*lmw->link.ilink = val;
	}
}

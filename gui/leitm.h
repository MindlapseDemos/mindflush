#ifndef LIBLEITMOTIF_H_
#define LIBLEITMOTIF_H_

#include <Xm/Xm.h>

typedef struct lmwidget lmwidget;
typedef void (*lm_event_handler)(Widget, void*, void*);


enum {LM_HORIZONTAL, LM_VERTICAL};


#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

lmwidget *lm_create_widget(void);
void lm_free_widget(lmwidget *w);
Widget lm_get_motif_widget(lmwidget *w);

lmwidget *lm_init(void);
lmwidget *lm_init_args(int *argc, char **argv);
void lm_show(lmwidget *w);
void lm_main_loop(void);

/* packing */
lmwidget *lm_vbox(lmwidget *parent);
lmwidget *lm_hbox(lmwidget *parent);
lmwidget *lm_radio_box(lmwidget *parent);

/* label */
lmwidget *lm_label(lmwidget *parent, const char *text);

/* textfield */
lmwidget *lm_textfield(lmwidget *parent, const char *text);
#define lm_textfield_link	lm_textfield_slink
lmwidget *lm_textfield_slink(lmwidget *parent, const char *text, char *link, size_t sz);
lmwidget *lm_textfield_ilink(lmwidget *parent, const char *text, int *link);
lmwidget *lm_textfield_flink(lmwidget *parent, const char *text, float *link);
lmwidget *lm_textfield_call(lmwidget *parent, const char *text, lm_event_handler handler);

void lm_textfield_set_text(lmwidget *w, const char *text);
const char *lm_textfield_get_text(lmwidget *w);

/* button */
lmwidget *lm_button(lmwidget *parent, const char *text);
lmwidget *lm_button_call(lmwidget *parent, const char *text, lm_event_handler handler);

/* scales */
lmwidget *lm_intscale(lmwidget *parent, int orien, int show, int min, int max, int val);
lmwidget *lm_intscale_link(lmwidget *parent, int orien, int show, int min, int max, int val, int *link);
lmwidget *lm_intscale_call(lmwidget *parent, int orien, int show, int min, int max, int val, lm_event_handler handler);

void lm_intscale_set_value(lmwidget *w, int val);
int lm_intscale_get_value(lmwidget *w);

lmwidget *lm_fltscale(lmwidget *parent, int orien, int show, float min, float max, float val);
lmwidget *lm_fltscale_link(lmwidget *parent, int orien, int show, float min, float max, float val, float *link);
lmwidget *lm_fltscale_call(lmwidget *parent, int orien, int show, float min, float max, float val, lm_event_handler handler);

void lm_fltscale_set_value(lmwidget *w, float val);
float lm_fltscale_get_value(lmwidget *w);

lmwidget *lm_progress(lmwidget *parent);

#define lm_progress_set_value(w, val)	lm_fltscale_set_value(w, val)

/* toggles */
lmwidget *lm_radio(lmwidget *parent, const char *text, int on);
lmwidget *lm_radio_link(lmwidget *parent, const char *text, int on, int *link);
lmwidget *lm_radio_call(lmwidget *parent, const char *text, int on, lm_event_handler handler);

void lm_radio_set_state(lmwidget *w, int val);
int lm_radio_get_state(lmwidget *w);

lmwidget *lm_check(lmwidget *parent, const char *text, int on);
lmwidget *lm_check_link(lmwidget *parent, const char *text, int on, int *link);
lmwidget *lm_check_call(lmwidget *parent, const char *text, int on, lm_event_handler handler);

void lm_check_set_state(lmwidget *w, int val);
int lm_check_get_state(lmwidget *w);

/* list */
lmwidget *lm_list(lmwidget *parent, int height, int msel);

void lm_list_append(lmwidget *w, const char *str);
void lm_list_remove(lmwidget *w, const char *str);

void lm_list_select(lmwidget *w, int idx);
int lm_list_get_selected(lmwidget *w);

void lm_list_select_many(lmwidget *w, int *idx);
int *lm_list_get_selected_many(lmwidget *w);

lmwidget *lm_separator(lmwidget *parent, int orientation);

/* dialogs */
enum {
	LM_INFO,
	LM_WARNING,
	LM_ERROR
};

#define lm_info(p, t)				lm_message(p, t, LM_INFO)
#define lm_info_call(p, t, c)		lm_message(p, t, LM_INFO, c)
#define lm_warning(p, t)			lm_message(p, t, LM_WARNING)
#define lm_warning_call(p, t, c)	lm_message(p, t, LM_WARNING, c)
#define lm_error(p, t)				lm_message(p, t, LM_ERROR)
#define lm_error_call(p, t, c)		lm_message(p, t, LM_ERROR, c)

void lm_message(lmwidget *parent, const char *text, int type);
void lm_message_call(lmwidget *parent, const char *text, int type, lm_event_handler handler);

void lm_file(lmwidget *parent, const char *mask);
void lm_file_call(lmwidget *parent, const char *mask, lm_event_handler handler);

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* LIBLEITMOTIF_H_ */

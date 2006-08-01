#include <iostream>
#include <cassert>
#include "3dengfx/3dengfx.hpp"
#include "common/timer.h"
#include "common/fps_counter.h"
#include "common/err_msg.h"
#include "dsys/demosys.hpp"
#include "events.h"
#include "demo_parts.h"

using namespace std;

bool init();
void clean_up();
void update_gfx();

ntimer timer;
fps_counter fps;

vector<dsys::Part*> parts;

static const char *title_str = "Mindlapse - Evoke 2006 Demo";

int main(int argc, char **argv) {
	if(!init()) {
		return -1;
	}

	return fxwt::main_loop();
}

bool init() {
	GraphicsInitParameters *gip;
	if(!(gip = load_graphics_context_config("3dengfx.conf"))) {
		return false;
	}
	
	if(!create_graphics_context(*gip)) {
		return false;
	}

	fxwt::set_window_title(title_str);

	fxwt::set_display_handler(update_gfx);
	fxwt::set_idle_handler(update_gfx);
	fxwt::set_keyboard_handler(key_handler);
	fxwt::set_motion_handler(motion_handler);
	fxwt::set_button_handler(bn_handler);
	atexit(clean_up);

	clear(0);
	fxwt::set_font_size(35);
	fxwt::set_font(fxwt::FONT_SERIF);
	fxwt::print_text("Mindlapse :: Evoke 2006", Vector2(0.25, 0.45), 0.08);
	fxwt::print_text("Loading...", Vector2(0.1, 0.90), 0.06, Color(0.2, 0.3, 0.9));
	flip();

	set_scene_data_path("data/img");

	ADD_ALL_PARTS();

	for(size_t i=0; i<parts.size(); i++) {
		add_part(parts[i]);
		info("adding part: %s", parts[i]->get_name());
	}

	dsys::start_demo();

	// start music
	/*
	if(sdlvf_init(music_fname) != SDLVF_PLAYING) {
		error("could not open music: %s", music_fname);
	}
	*/

	timer_reset(&timer);
	timer_start(&timer);

	fps_start(&fps, 0, 1000);
	return true;
}

void update_gfx() {
	// sdlvf_check();
	
	if(dsys::update_graphics() == -1) exit(0);

	if(fps_frame_proc(&fps, timer_getmsec(&timer))) {
		fprintf(stderr, "fps: %f\r", fps_get_frame_rate(&fps));
	}
}

void clean_up() {
	for(size_t i=0; i<parts.size(); i++) {
		delete parts[i];
	}
	destroy_graphics_context();

	fputc('\n', stderr);
}

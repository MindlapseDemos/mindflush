#include <iostream>
#include <cassert>
#include <SDL/SDL.h>
#include "3dengfx/3dengfx.hpp"
#include "common/timer.h"
#include "common/fps_counter.h"
#include "common/err_msg.h"
#include "dsys/demosys.hpp"
#include "events.h"
#include "demo_parts.h"
#include "sdlvf.h"

using namespace std;

bool init();
void clean_up();
void update_gfx();

ntimer timer;
fps_counter fps;

bool play_music = true;	// XXX: change this before release :)
const char *music_fname = "data/bassbandit_music.ogg";

vector<dsys::Part*> parts;

static const char *title_str = "Mindflush / MLFC - PixelShow 2007";

bool render_demo = false;
const char *render_path = "frames";

int main(int argc, char **argv) {
	for(int i=1; i<argc; i++) {
		if(argv[i][0] == '-' && argv[i][2] == 0) {
			switch(argv[i][1]) {
			case 'm':
				play_music = !play_music;
				break;

			case 'r':
				render_demo = true;
				break;

			case 'p':
				render_path = argv[++i];
				break;

			default:
				cerr << "invalid option: " << argv[i] << endl;
				return EXIT_FAILURE;
			}
		} else {
			cerr << "unexpected argument: " << argv[i] << endl;
			return EXIT_FAILURE;
		}
	}

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
	SDL_ShowCursor(0);

	fxwt::set_display_handler(update_gfx);
	fxwt::set_idle_handler(update_gfx);
	fxwt::set_keyboard_handler(key_handler);
	fxwt::set_motion_handler(motion_handler);
	fxwt::set_button_handler(bn_handler);
	atexit(clean_up);

#if defined(WIN32) || defined(__WIN32__)
	dsys::overlay(get_texture("data/img/loading_w32.jpg"), Vector2(0, 0), Vector2(1, 1), 1.0);
#else
	dsys::overlay(get_texture("data/img/loading.jpg"), Vector2(0, 0), Vector2(1, 1), 1.0);
#endif
	flip();

	set_scene_data_path("data/img");

	ADD_ALL_PARTS();

	for(size_t i=0; i<parts.size(); i++) {
		add_part(parts[i]);
		info("adding part: %s", parts[i]->get_name());
	}

	if(!render_demo) {
		dsys::start_demo();
	} else {
		dsys::render_demo(25, render_path);
	}

	// start music
	if(play_music && sdlvf_init(music_fname) != SDLVF_PLAYING) {
		error("could not open music: %s", music_fname);
	}

	timer_reset(&timer);
	timer_start(&timer);

	fps_start(&fps, 0, 1000);
	return true;
}

void update_gfx() {
	if(play_music) sdlvf_check();
	
	if(dsys::update_graphics() == -1) exit(0);

	if(fps_frame_proc(&fps, timer_getmsec(&timer))) {
		fprintf(stderr, "fps: %f\r", fps_get_frame_rate(&fps));
	}
}

void clean_up() {
	static bool clean_up_called = false;

	SDL_ShowCursor(1);

	if(clean_up_called) {
		warning("Multiple clean_up() calls");
		return;
	} else {
		clean_up_called = true;
	}

	for(size_t i=0; i<parts.size(); i++) {
		delete parts[i];
	}
	destroy_graphics_context();

	fputc('\n', stderr);
}

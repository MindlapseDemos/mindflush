#include <iostream>
#include "events.h"
#include "3dengfx/3dengfx.hpp"

using namespace engfx_state;

void key_handler(int key) {
	/*
	static bool running = true;
	extern ntimer timer;
	extern Scene *scene;
	*/
	extern float foo;
	char str[128];
	TargetCamera *cam = dynamic_cast<TargetCamera*>(const_cast<Camera*>(view_mat_camera));
	
	switch(key) {
	case 'a':
		if(cam) cam->zoom(0.8);
		break;

	case 'z':
		if(cam) cam->zoom(1.2);
		break;

	case '`':
		screen_capture();
		break;

	case '-':
		foo -= 0.01;
		if(foo < 0.0) foo = 0.0;
		sprintf(str, "%f", foo);
		fxwt::set_window_title(str);
		break;

	case '=':
		foo += 0.01;
		if(foo > 0.98) foo = 0.98;
		sprintf(str, "%f", foo);
		fxwt::set_window_title(str);
		break;

	/*
	case ' ':
		if(running) {
			timer_stop(&timer);
		} else {
			timer_start(&timer);
		}
		running = !running;
		break;

	case fxwt::KEY_LEFT:
		timer_back(&timer, 500);
		break;

	case fxwt::KEY_RIGHT:
		timer_fwd(&timer, 500);
		break;

	case fxwt::KEY_DOWN:
		timer_back(&timer, 5000);
		break;

	case fxwt::KEY_UP:
		timer_fwd(&timer, 5000);
		break;
	*/

	case fxwt::KEY_ESCAPE:
	case 'q':
		exit(0);

	default:
		break;
	}
}

static int prev_x = -1;
static int prev_y = -1;

void motion_handler(int x, int y) {
	if(prev_x != -1) {
		TargetCamera *cam = dynamic_cast<TargetCamera*>(const_cast<Camera*>(view_mat_camera));
		if(cam) {
			float dx = (float)(x - prev_x) * 0.01;
			float dy = (float)(y - prev_y) * 0.25;
		
			cam->rotate(Vector3(0, -dx, 0));
			cam->translate(Vector3(0, -dy, 0));
		}
		
		prev_x = x;
		prev_y = y;
	}
}

void bn_handler(int bn, int pressed, int x, int y) {
	TargetCamera *cam = dynamic_cast<TargetCamera*>(const_cast<Camera*>(view_mat_camera));
	
	switch(bn) {
	case fxwt::BN_RIGHT:
		if(pressed) {
			prev_x = x;
			prev_y = y;
		} else {
			prev_x = prev_y = -1;
		}
		break;

	case fxwt::BN_WHEELUP:
		if(cam) cam->zoom(0.9);
		break;

	case fxwt::BN_WHEELDOWN:
		if(cam) cam->zoom(1.1);
		break;

	default:
		break;
	}
}

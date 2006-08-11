#include <vector>
#include "p_plat.h"

using namespace std;
using namespace dsys;

static std::vector<RendCurve*> vines;

static Object *ground;
static TargetCamera *cam, *mirror_cam;

PlatPart::PlatPart() : ScenePart("plat", "data/geom/6plat.3ds") {
	Curve *spline;
	for(int i=0; i<4; i++) {	
		char name[16];
		sprintf(name, "Vine%02d", i + 1);
		std::cerr << "trying " << name << std::endl;
		if((spline = scene->get_curve(name))) {
			spline->set_arc_parametrization(true);
			RendCurve *rcurve = new RendCurve(spline);
			rcurve->set_width(0.5);
			rcurve->mat.diffuse_color = Color(0.2, 0.9, 0.4);
			rcurve->mat.set_texture(get_texture("data/img/pulse2.png"), TEXTYPE_DIFFUSE);
			vines.push_back(rcurve);
			std::cout << "OK\n";//got vine: " << name << std::endl;
		} else {
			std::cout << "Not found\n";
		}
	}

	ground = scene->get_object("Plane01");
	//scene->remove_object(ground);

	cam = (TargetCamera*)scene->get_active_camera();

	Vector3 cam_pos = cam->get_position(0);
	Vector3 cam_targ = cam->target.get_position(0);

	Vector3 mir_pos = Vector3(cam_pos.x, -cam_pos.y, cam_pos.z);
	Vector3 mir_targ = Vector3(cam_targ.x, -cam_targ.y, cam_targ.z);

	mirror_cam = new TargetCamera(mir_pos, mir_targ);
	mirror_cam->flip(false, true, false);
	mirror_cam->zoom(0.74);
	//scene->add_camera(mirror_cam);

	scene->set_auto_clear(false);
}

PlatPart::~PlatPart() {
	for(size_t i=0; i<vines.size(); i++) {
		delete vines[i];
	}
}

void PlatPart::draw_part() {
//	float plane_eq = {};
	float t = (float)time / 1000.0;
/*
	cam->activate(time);
	scene->setup_lights();

	clear_stencil(0);
	//set_color_write(false, false, false, false);
	glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
	set_stencil_op(SOP_INC, SOP_INC, SOP_INC);
	glEnable(GL_STENCIL_TEST);
	ground->render(time);

	clear_zbuffer(1.0);

	scene->set_active_camera(mirror_cam);
	set_color_write(true, true, true, true);
	glStencilFunc(GL_EQUAL, 1, 0xffffffff);
	set_stencil_op(SOP_KEEP, SOP_KEEP, SOP_KEEP);
//	glEnable(GL_CLIP_PLANE0);
	scene->render(time);
//	glDisable(GL_CLIP_PLANE0);
	scene->set_active_camera(cam);

	clear_zbuffer(1.0);
	
	glDisable(GL_STENCIL_TEST);
	scene->render(time);
*/
	scene->render(time);

	/*for(size_t i=0; i<vines.size(); i++) {
		vines[i]->render_segm(0.0, (t * 0.25) > 1.0 ? 1.0 : (t * 0.25));
	}*/

}

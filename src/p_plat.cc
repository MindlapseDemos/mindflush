#include <vector>
#include "p_plat.h"

using namespace std;
using namespace dsys;

static void make_skycube(Scene *scene);
static void make_psys(Scene *scene);

//static Object *ground;
static TargetCamera *cam;//, *mirror_cam;
static XFormNode *targ;

static vector<ParticleSystem*> particles;

PlatPart::PlatPart() : ScenePart("plat", "data/geom/6plat.3ds") {
	make_skycube(scene);
	make_psys(scene);

	cam = (TargetCamera*)scene->get_active_camera();
	cam->set_clipping_plane(50000, CLIP_FAR);
	cam->set_position(Vector3(0, 0, 0));
	cam->target.set_position(Vector3(0, 0, 0));
	cam->set_fov(DEG_TO_RAD(55.0));

	Object *dummy = scene->get_object("Sphere01");
	scene->remove_object(dummy);

	dummy->children.push_back(targ);
	cam->target.parent = dummy;

	GfxProg *phong_prog = new GfxProg(get_shader("sdr/phong_v.glsl", PROG_VERTEX), get_shader("sdr/phong2_p.glsl", PROG_PIXEL));
	phong_prog->link();
	if(!phong_prog->is_linked()) {
		exit(0);
	}

	std::list<Object*> *objlist = scene->get_object_list();
	std::list<Object*>::iterator iter = objlist->begin();
	while(iter != objlist->end()) {
		Object *obj = *iter++;
		if(obj->mat.name == "01 - Default") {
			obj->set_gfx_program(phong_prog);
		}
	}

	/*
	ground = scene->get_object("Plane01");
	scene->remove_object(ground);
	cam = (TargetCamera*)scene->get_active_camera();

	Vector3 cam_pos = cam->get_position(0);
	Vector3 cam_targ = cam->target.get_position(0);

	Vector3 mir_pos = Vector3(cam_pos.x, -cam_pos.y, cam_pos.z);
	Vector3 mir_targ = Vector3(cam_targ.x, -cam_targ.y, cam_targ.z);

	mirror_cam = new TargetCamera(mir_pos, mir_targ);
	mirror_cam->flip(false, true, false);
	scene->add_camera(mirror_cam);
	*/

	scene->set_auto_clear(false);
}

PlatPart::~PlatPart() {
	for(size_t i=0; i<particles.size(); i++) {
		delete particles[i];
	}
	particles.clear();
}

void PlatPart::draw_part() {
	scene->render(time);// * 0.7);

	psys::set_global_time(time);
	for(int i=0; i<particles.size(); i++) {
		particles[i]->update();
		particles[i]->draw();
	}

/*
//	float plane_eq = {};
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

	/*for(size_t i=0; i<vines.size(); i++) {
		vines[i]->render_segm(0.0, (t * 0.25) > 1.0 ? 1.0 : (t * 0.25));
	}*/

}


static void make_skycube(Scene *scene) {
	const float size = 40000;
	Object *face[6];
	Texture *tex[6];

	face[0] = new ObjPlane(Vector3(0, -1, 0), Vector2(size, size), 0);
	face[0]->translate(Vector3(0, size / 2, 0));
	tex[0] = get_texture("data/img/py.jpg");
	
	face[1] = new ObjPlane(Vector3(0, 1, 0), Vector2(size, size), 0);
	face[1]->translate(Vector3(0, -size / 2, 0));
	tex[1] = get_texture("data/img/ny.jpg");

	face[2] = new ObjPlane(Vector3(0, 0, -1), Vector2(size, size), 0);
	face[2]->translate(Vector3(0, 0, size / 2));
	tex[2] = get_texture("data/img/pz.jpg");
	
	face[3] = new ObjPlane(Vector3(0, 0, 1), Vector2(size, size), 0);
	face[3]->translate(Vector3(0, 0, -size / 2));
	tex[3] = get_texture("data/img/nz.jpg");

	face[4] = new ObjPlane(Vector3(-1, 0, 0), Vector2(size, size), 0);
	face[4]->translate(Vector3(size / 2, 0, 0));
	tex[4] = get_texture("data/img/px.jpg");
	
	face[5] = new ObjPlane(Vector3(1, 0, 0), Vector2(size, size), 0);
	face[5]->translate(Vector3(-size / 2, 0, 0));
	tex[5] = get_texture("data/img/nx.jpg");

	for(int i=0; i<6; i++) {
		Material *mat = face[i]->get_material_ptr();
		mat->emissive_color = 1.0;
		//add_texture(tex[i]);
		if(i != 1) {
			mat->set_texture(tex[i], TEXTYPE_DIFFUSE);
			face[i]->set_texture_addressing(TEXADDR_CLAMP);
			scene->add_object(face[i]);
		}
	}
}

static void make_psys(Scene *scene) {
	MotionController ctrl;
	ParticleSystem *ps;
	// ---- fire ----
	ps = new ParticleSystem;
	if(!psys::load_particle_sys_params("data/psys/fire.psys", ps->get_params())) {
		exit(0);
	}
	ps->set_position(Vector3(0, 35, 71));
	//scene->add_particle_sys(ps);
	particles.push_back(ps);

	// ---- magic (red) ----
	ps = new ParticleSystem;
	if(!psys::load_particle_sys_params("data/psys/red.psys", ps->get_params())) {
		exit(0);
	}
	ps->set_position(Vector3(0, 80.0, 0));
	//scene->add_particle_sys(ps);
	particles.push_back(ps);

	ctrl = MotionController(CTRL_SIN, TIME_FREE);
	ctrl.set_control_axis(CTRL_X);
	ctrl.set_sin_func(1.0, 100.0);
	ps->add_controller(ctrl, CTRL_TRANSLATION);

	ctrl = MotionController(CTRL_COS, TIME_FREE);
	ctrl.set_control_axis(CTRL_Z);
	ctrl.set_sin_func(1.5, 75.0);
	ps->add_controller(ctrl, CTRL_TRANSLATION);

	ctrl = MotionController(CTRL_SIN, TIME_FREE);
	ctrl.set_control_axis(CTRL_Y);
	ctrl.set_sin_func(1.8, 20.0);
	ps->add_controller(ctrl, CTRL_TRANSLATION);
}

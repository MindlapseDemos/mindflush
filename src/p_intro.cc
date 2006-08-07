#include "p_intro.h"

#define MLCURVE 	"data/curves/mlapse.curve"

static Curve *mlapse;
static RendCurve *rend_mlapse;
static ParticleSystem *mlpsys;
static Fuzzy orig_birth_rate;

static const int pulse_dur = 3000;

IntroPart::IntroPart() : ScenePart("intro", (Scene*)0) {
	scene = new Scene;
	scene->add_camera(new TargetCamera(Vector3(0, 10, -150)));
	scene->add_light(new PointLight(Vector3(-100, 100, -100)));

	if(!(mlapse = load_curve(MLCURVE))) {
		exit(0);
	}
	mlapse->set_arc_parametrization(true);

	rend_mlapse = new RendCurve(mlapse);
	rend_mlapse->set_width(0.1);

	mlpsys = new ParticleSystem;
	if(!psys::load_particle_sys_params("data/psys/mlapse.psys", mlpsys->get_params())) {
		exit(0);
	}
	mlpsys->add_controller(MotionController(mlapse, 0, pulse_dur, TIME_CLAMP), CTRL_TRANSLATION);
	mlpsys->set_update_interval(1.0 / 120.0);
	//scene->add_particle_sys(mlpsys);
	orig_birth_rate = mlpsys->get_params()->birth_rate;

	// hal plane
	ObjPlane *hal = new ObjPlane(Vector3(0, 0, 1), Vector2(100, 100), 0);
	hal->mat.set_texture(get_texture("data/img/hal1.jpg"), TEXTYPE_DIFFUSE);
	scene->add_object(hal);
}

IntroPart::~IntroPart() {
	delete rend_mlapse;
	delete mlapse;
}

void IntroPart::draw_part() {
	static unsigned long ptime, prev_ptime;
	float t = (float)time / 1000.0;
	scene->render(time);
	//rend_mlapse->render(time);

	ptime = time % pulse_dur * 4;
	if(ptime > pulse_dur) {
		mlpsys->get_params()->birth_rate = 0.0;
	}
	if(ptime < prev_ptime) {
		mlpsys->reset();
		mlpsys->get_params()->birth_rate = orig_birth_rate;
	}
	psys::set_global_time(ptime);
	mlpsys->update();
	mlpsys->draw();
	prev_ptime = ptime;
}

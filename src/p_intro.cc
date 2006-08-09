#include "p_intro.h"

#define MLCURVE 	"data/curves/mlapse.curve"

static Vector3 curve_xform(const Vector3 &pt);

static Curve *mlapse;
static RendCurve *rend_mlapse;
static ParticleSystem *mlpsys;
static Fuzzy orig_birth_rate;

static const int pulse_dur = 2000;
static const float compl_time = 10.0;

static float t;

IntroPart::IntroPart() : ScenePart("intro", (Scene*)0) {
	scene = new Scene;
	scene->add_camera(new TargetCamera(Vector3(0, 10, -150)));
	scene->add_light(new PointLight(Vector3(-100, 100, -100)));

	if(!(mlapse = load_curve(MLCURVE))) {
		exit(0);
	}
	mlapse->set_arc_parametrization(true);
	mlapse->set_xform_func(curve_xform);

	rend_mlapse = new RendCurve(mlapse);
	rend_mlapse->set_width(0.75);
	rend_mlapse->get_material_ptr()->diffuse_color = Color(56.0 / 255.0, 1.0, 75.0 / 255.0);
	rend_mlapse->set_blending_mode(BLEND_ONE, BLEND_ONE);
	rend_mlapse->get_material_ptr()->set_texture(get_texture("data/img/pulse.png"), TEXTYPE_DIFFUSE);

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

#define MIN(a, b)	((a) < (b) ? (a) : (b))

void IntroPart::draw_part() {
	static unsigned long ptime, prev_ptime;
	t = (float)time / 1000.0;
	scene->render(time);

	ptime = (time % pulse_dur);
	if(ptime < pulse_dur) {
		float start = (float)ptime / pulse_dur;
		float end = MIN((float)(ptime + pulse_dur / 5) / pulse_dur, 1.0);
		rend_mlapse->render_segm(start, end);
	}

	/*
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
	*/
}

static float psin(float x) {
	return (sin(x) + 1.0) / 2.0;
}

static Vector3 curve_xform(const Vector3 &pt) {
	float progr = MIN(t / compl_time, 1.0);
	float p = (psin(t * 10.0 *pt.x ) + psin(cos(pt.x) + t * 13.0)) / 2.0;
	p *= psin(cos(pt.x / 10.0 + t) * 5.0);
	p = lerp(p, 1.0, progr);
	return Vector3(pt.x, pt.y * p, pt.z);
}

#include "p_dcurve.h"

static Curve *curve0, *curve1;
static RendCurve *rend_curve0, *rend_curve1;
static float t;
static float offset, mult, xoffset;

static const float fade_out = 8;
static const float fade_dur = 2;
static const float curve_completion = 5.0f;

static float calpha()
{
	if (t < fade_dur)
	{
		return (t / fade_dur);
	}
	else if (t >= fade_out)
	{
		float ret = 1 - (t - fade_out) / fade_dur;
		if (ret < 0) ret = 0;
		return ret;
	}
	return 1;
}

static float cparam()
{
	float ret = t / curve_completion;
	if (ret > 1.0f)
	{
		return 1.0f;
	}
	return ret;
}

Vector3 curve0_xform(const Vector3 &pt)
{
	Matrix4x4 xform;
	xform.reset_identity();

	float p = cparam();

	float sc = 4 + p;

	xform.translate(Vector3(-250 + xoffset, -50 + offset, 0));
	xform.scale(Vector3(sc * (1 + t*mult/15), sc * t * mult + sin(pt.x/ 10 + mult), sc));
	xform.rotate(Vector3(0, 0, 1), pi / 2 - 0.02f * t);

	return pt.transformed(xform);
}

DCurvePart::DCurvePart() : ScenePart("dcurve", (Scene*)0) {
	
	scene = new Scene;
	scene->add_camera(new TargetCamera(Vector3(0, 0, -1000)));
	scene->set_auto_clear(false);
	
	curve0 = load_curve("data/curves/design00");
	if (!curve0)
	{
		printf("PAPARIA!\n");
		exit(0);
	}
	curve0->set_arc_parametrization(true);
	curve0->set_xform_func(curve0_xform);
	rend_curve0 = new RendCurve(curve0);
	rend_curve0->set_width(3.5);
	rend_curve0->get_material_ptr()->diffuse_color = Color(0.3, 0.4, 1.0);
	rend_curve0->set_blending_mode(BLEND_ONE, BLEND_ONE);
	rend_curve0->get_material_ptr()->set_texture(get_texture("data/img/pulse2.png"), TEXTYPE_DIFFUSE);

	curve1 = load_curve("data/curves/design01");
	if (!curve1)
	{
		printf("PAPARIA!\n");
		exit(0);
	}
	curve1->set_arc_parametrization(true);
	curve1->set_xform_func(curve0_xform);
	rend_curve1 = new RendCurve(curve1);
	rend_curve1->set_width(3.5);
	rend_curve1->get_material_ptr()->diffuse_color = Color(0.3, 0.4, 1.0);
	rend_curve1->set_blending_mode(BLEND_ONE, BLEND_ONE);
	rend_curve1->get_material_ptr()->set_texture(get_texture("data/img/pulse2.png"), TEXTYPE_DIFFUSE);

}

DCurvePart::~DCurvePart() {
}

void DCurvePart::draw_part() {

	// update time
	t = time / 1000.0f;

	scene->render();
	float start = fmodf(cparam(), 1);
	
	Color clr = Color(0.3, 0.4, 1.0);

	rend_curve0->mat.diffuse_color = clr * calpha();

	xoffset = -100;
	srand(0);
	set_zbuffering(false);
	for (int i=0; i< 10; i++)
	{
		offset = 200 * frand(1);
		mult = frand(2) -1; 
		rend_curve0->render_segm(0, cparam());
	}

	rend_curve1->mat.diffuse_color = clr * calpha();

	xoffset = 650;
	for (int i=0; i< 10; i++)
	{
		offset = 200 * frand(1);
		mult = frand(2) -1; 
		rend_curve1->render_segm(0, cparam());
	}
	set_zbuffering(true);
}

#include "p_tunnel.h"

static ObjPlane * tplane;
static GfxProg *tunnel_prog;

static const int num_planes = 40;
static const float plane_interval = 15.0;
static const float farthest = num_planes * plane_interval;

float tunnel_duration = 5.0;

TunnelPart::TunnelPart() : ScenePart("tunnel", (Scene*)0) {
	scene = new Scene;
	scene->add_camera(new TargetCamera(Vector3(0, 0, 0), Vector3(0, 0, 10)));
	scene->add_light(new PointLight(Vector3(-100, 100, -100)));

	tplane = new ObjPlane(Vector3(0, 0, -1), Vector2(100, 100), 0);
	tplane->mat.alpha = 0.999;
	tplane->set_zwrite(false);
	tplane->set_blending(true);
	//tplane->set_blending_mode(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA);
	tplane->set_blending_mode(BLEND_SRC_ALPHA, BLEND_ONE);
	//tplane->set_handle_blending(false);

	// setup shaders
	tunnel_prog = new GfxProg(get_shader("sdr/tunnel_v.glsl", PROG_VERTEX), get_shader("sdr/tunnel_p.glsl", PROG_PIXEL));
	tunnel_prog->link();
	if(!tunnel_prog->is_linked()) {
		exit(0);
	}
	tunnel_prog->set_parameter("farthest", farthest);

	tplane->set_gfx_program(tunnel_prog);

	// cache textures
	tplane->mat.set_texture(get_texture("data/img/04.png"), TEXTYPE_DIFFUSE);
}

TunnelPart::~TunnelPart() {
}

void TunnelPart::draw_part() {
	float t = (float)time / 1000.0;

	scene->render();

	// render planes
	float the_offs = (t - tunnel_duration) * 100;
	if (t < tunnel_duration)
	{
		the_offs = fmod(the_offs, plane_interval);
	}

	for (int i=0; i<num_planes; i++)
	{
		float offs = i * plane_interval + the_offs;
		float pos = farthest - offs;
		tunnel_prog->set_parameter("t", pos / 100.0f + t * 2.0f);

		float depth = pos / farthest;
		float xmot = 125.0 * (sin(t * 1.5 + depth) + cos(t * 2.0 + depth) * 0.6) * depth;
		float ymot = 125.0 * (cos(t * 1.5 + depth) + sin(t * 2.0 + depth) * 0.6) * depth;

		tplane->set_position(Vector3(xmot, ymot, pos));
		tplane->render();
	}
}

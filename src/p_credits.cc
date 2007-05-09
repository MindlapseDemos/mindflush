#include "p_credits.h"
#include "3dengfx/object.hpp"

struct Design {
	Object *obj;
	Texture *tex;
	float sz;
	Vector3 pos;
	float start_t;
	float rot;
};

static Texture *cred, *cred_blur;
static Object *cyl;
static GfxProg *cylprog, *desprog;

#define DES_COUNT	1
static Design des[DES_COUNT];

#define DTEX_COUNT	1
static char *dtex_name[DTEX_COUNT] = {
	"data/img/des01.png"
};

CreditsPart::CreditsPart() : ScenePart("credits", (Scene*)0)
{
	Shader vsdr, psdr;

	scene = new Scene;
	scene->set_auto_clear(false);
	scene->add_camera(new TargetCamera(Vector3(0, 10, -130)));
	scene->add_light(new PointLight(Vector3(-100, 100, -100)));

	if(!(cred = get_texture("data/img/credits.png"))) {
		exit(0);
	}
	if(!(cred_blur = get_texture("data/img/credits_blur.png"))) {
		exit(0);
	}

	if(!(vsdr = get_shader("sdr/credtxt_v.glsl", PROG_VERTEX))) {
		exit(0);
	}
	if(!(psdr = get_shader("sdr/credtxt_p.glsl", PROG_PIXEL))) {
		exit(0);
	}
	cylprog = new GfxProg(vsdr, psdr);
	cylprog->link();
	if(!cylprog->is_linked()) {
		exit(0);
	}

	cyl = new ObjCylinder(60.0, 40.0, false, 80);
	cyl->set_position(Vector3(-19, 0, 0));
	cyl->set_rotation(Vector3(0, 0, half_pi));
	cyl->rotate(Vector3(0.1, 0, 0));
	cyl->get_material_ptr()->set_texture(cred, TEXTYPE_DIFFUSE);
	cyl->get_material_ptr()->set_texture(cred_blur, TEXTYPE_DETAIL);
	cyl->set_gfx_program(cylprog);
	
	scene->add_object(cyl);

	// --------------------------------------
	
	if(!(vsdr = get_shader("sdr/design_v.glsl", PROG_VERTEX))) {
		exit(0);
	}
	if(!(psdr = get_shader("sdr/design_p.glsl", PROG_PIXEL))) {
		exit(0);
	}
	desprog = new GfxProg(vsdr, psdr);
	desprog->link();
	if(!desprog->is_linked()) {
		exit(0);
	}
	
	for(int i=0; i<DES_COUNT; i++) {
		if(!(des[i].tex = get_texture(dtex_name[0]))) {
			exit(0);
		}
		des[i].sz = 20.0;
		des[i].start_t = 0.0;
		des[i].pos = Vector3(0, 0, 0);
		des[i].rot = 0.0;
		//des[i].obj = ObjPlane(Vector3(0, 0, -1), Vector2(des[i].sz, des[i].sz));
	}
}

CreditsPart::~CreditsPart()
{
}

void CreditsPart::draw_part()
{
	float t = (float)time / 1000.0f;

	clear(Color(0, 0, 0, 1));
	clear_zbuffer_stencil(1.0, 0);

	scene->get_active_camera()->activate(0);

	set_alpha_blending(true);
	set_blend_func(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA);
	for(int i=0; i<DES_COUNT; i++) {
		desprog->set_parameter("compl", t / 10.0f);
		set_gfx_program(desprog);
		set_texture(0, des[i].tex);
		draw_point(des[i].pos, des[i].sz);
		set_gfx_program(0);
	}
	set_alpha_blending(false);

	// ---- scene ----
	
	glext::glActiveTexture(GL_TEXTURE0);
	set_texture(0, get_texture("data/img/credits.png"));
	cylprog->set_parameter("text", 0);
	glext::glActiveTexture(GL_TEXTURE1);
	set_texture(1, get_texture("data/img/credits_blur.png"));
	cylprog->set_parameter("text_blur", 1);

	float glowp = fabs(sin(t * 5.0)) * fabs(sin(t * 5.2));
	cylprog->set_parameter("glow_param", glowp);
	cylprog->set_parameter("t", t);

	scene->render(0);

	glext::glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glext::glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

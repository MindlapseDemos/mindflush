#include "p_credits.h"
#include "3dengfx/object.hpp"

#define PDUR	45000

struct Design {
	Object *obj;
	Vector3 pos;
	Color col;
	float start_t;
	float rot;
};

static Texture *cred, *cred_blur;
static Object *cyl;
static GfxProg *cylprog, *desprog, *blurprog;

#define DES_COUNT	45
static Design des[DES_COUNT];

#define DOBJ_COUNT	3
static Object *dmodel[DOBJ_COUNT];
static char *dmodel_name[] = {"des_sq", "des_sp", "des_fl"};

CreditsPart::CreditsPart() : ScenePart("credits", (Scene*)0)
{
	Shader vsdr, psdr;

	if(!(scene = load_scene("data/geom/designs.3ds"))) {
		exit(0);
	}
	scene->set_auto_clear(false);
	scene->add_camera(new TargetCamera(Vector3(0, 0, -130)));
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
	cyl->set_zwrite(false);
	cyl->set_blending(true);
	cyl->set_blending_mode(BLEND_ONE, BLEND_ONE);
	cyl->set_handle_blending(false);
	
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

	Color col1 = int_color(182, 155, 95);
	Color col2 = int_color(110, 190, 202);
	for(int i=0; i<DOBJ_COUNT; i++) {
		dmodel[i] = scene->get_object(dmodel_name[i]);
		dmodel[i]->set_gfx_program(desprog);
		dmodel[i]->set_zwrite(false);
		dmodel[i]->set_blending(true);
		dmodel[i]->set_blending_mode(BLEND_ONE, BLEND_ONE);
		dmodel[i]->set_handle_blending(false);
		dmodel[i]->get_material_ptr()->diffuse_color = blend_colors(col1, col2, frand(1.0));
		scene->remove_object(dmodel[i]);
	}
	
	for(int i=0; i<DES_COUNT; i++) {
		des[i].obj = dmodel[rand() % DOBJ_COUNT];
		des[i].start_t = (rand() % PDUR) / 1000.0f;
		des[i].pos = Vector3(0, frand(160.0f) - 80.0f, frand(140.0f) + 40.0f);
		des[i].rot = frand(0.5) - 0.25;
	}

	if(!(psdr = get_shader("sdr/gblur_p.glsl", PROG_PIXEL))) {
		exit(0);
	}
	blurprog = new GfxProg(vsdr, psdr);
	blurprog->link();
	if(!blurprog->is_linked()) {
		exit(0);
	}
}

CreditsPart::~CreditsPart()
{
}

#define MAX(a, b)	((a) > (b) ? (a) : (b))
#define MAX_OFFS	140
void CreditsPart::draw_part()
{
	float t = (float)time / 1000.0f;
	scene->get_active_camera()->activate(0);

	clear(Color(0, 0, 0, 1));
	clear_zbuffer_stencil(1.0, 0);

	set_render_target(dsys::tex[0]);
	clear(Color(0, 0, 0, 1));
	clear_zbuffer_stencil(1.0, 0);

	for(int i=0; i<DES_COUNT; i++) {
		desprog->set_parameter("compl", (t - des[i].start_t) / 16.0f);

		float x = -(2.0 * MAX_OFFS * (t - des[i].start_t) / 25.0f - MAX_OFFS);
		des[i].obj->set_position(des[i].pos + Vector3(x, 0.0, 0.0));
		des[i].obj->set_rotation(Vector3(0, 0, t * des[i].rot));
		des[i].obj->get_material_ptr()->diffuse_color.a = x / (float)MAX_OFFS + 0.6;
		des[i].obj->render(0);
	}
	set_render_target(0);

	set_alpha_blending(true);
	set_blend_func(BLEND_ONE, BLEND_ONE);
	dsys::overlay(dsys::tex[0], Vector2(0, 0), Vector2(1, 1), Color(1, 1, 1, 1), blurprog, false);
	dsys::overlay(dsys::tex[0], Vector2(0, 0), Vector2(1, 1), Color(1, 1, 1, 1), blurprog, false);
	set_alpha_blending(false);

	// ---- scene ----
	
	glext::glActiveTexture(GL_TEXTURE0);
	set_texture(0, get_texture("data/img/credits.png"));
	cylprog->set_parameter("text", 0);
	glext::glActiveTexture(GL_TEXTURE1);
	set_texture(1, get_texture("data/img/credits_blur.png"));
	cylprog->set_parameter("text_blur", 1);

	float glowp = fabs(sin(t * 5.0)) * fabs(sin(t * 5.4));
	cylprog->set_parameter("glow_param", glowp);
	cylprog->set_parameter("t", t * 1.1f);

	scene->render(0);

	glext::glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glext::glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

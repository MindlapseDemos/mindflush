#include <cassert>
#include "p_box.h"

struct Blob {
	Vector3 pos;
	float energy;
	XFormNode *xflink;
};

const static int bcount = 4;
static Blob blob[bcount];

static ScalarField *sf, *sf_flat;
static const int grid = 26;
static const float fieldsz = 11.3;
static const float wall_dist = 6.75;
static const float iso_val = 0.8;
	
static Object *blobj;
static GfxProg *gfx_prog;

static void update_blobs(unsigned long time);
static float eval_func(const Vector3 &vec, float t);

float foo = 0.0;

BoxPart::BoxPart() : ScenePart("box", "data/geom/cube-traj.3ds") {
	scene->set_ambient_light(Color(0.1, 0.1, 0.1));

	for(int i=0; i<bcount; i++) {
		char objname[] = "Sphere0x";
		objname[strlen(objname) - 1] = '1' + i;
		Object *obj = scene->get_object(objname);
		assert(obj);
		blob[i].xflink = obj;
		blob[i].energy = obj->mesh.get_vertex_stats().max_dist;

		obj->set_timeline_mode(TIME_WRAP);
		obj->set_hidden(true);
	}

	Camera *cam = scene->get_active_camera();
	cam->set_timeline_mode(TIME_WRAP);
	cam->set_fov(DEG_TO_RAD(30.0));

	Object *cube = scene->get_object("Box01");
	cube->set_hidden(true);

	// initialize the scalar fields
	float hsz = fieldsz / 2.0;
	
	sf = new ScalarField(grid, Vector3(-hsz, -hsz, -hsz), Vector3(hsz, hsz, hsz));
	sf->set_evaluator(eval_func);
	
	sf_flat = new ScalarField(grid * 0.75, Vector3(-hsz, -hsz, -hsz), Vector3(hsz, hsz, hsz));
	sf_flat->set_evaluator(eval_func);

	// load the phong shader program
	Shader phong_pixel = get_shader("sdr/phong_p.glsl", PROG_PIXEL);
	Shader phong_vertex = get_shader("sdr/phong_v.glsl", PROG_VERTEX);
	if(!phong_pixel || !phong_vertex) exit(EXIT_FAILURE);
	gfx_prog = new GfxProg(phong_vertex, phong_pixel);

	blobj = new Object;
	blobj->set_dynamic(true);
	blobj->mat.diffuse_color = Color(0.074, 0.52, 0.83);
	blobj->mat.specular_color = 1.0;
	blobj->mat.specular_power = 60.0;
	blobj->set_gfx_program(gfx_prog);
	blobj->set_texture_addressing(TEXADDR_CLAMP);
	blobj->set_highlight(true);
	blobj->set_highlight_color(Color(0.3, 0.3, 0.3));
	blobj->mat.set_texture(get_texture("data/img/cont_cel_pal.png"), TEXTYPE_DIFFUSE);
	scene->add_object(blobj);
}

BoxPart::~BoxPart() {
}

void BoxPart::draw_part() {
	float t = (float)time / 1000.0;
	float foo = sin(t / 2.0) * 0.5 + 0.5;
	update_blobs(time);
	
	sf->triangulate(&blobj->mesh, iso_val, t * 0.75, true);
	blobj->mesh.invert_winding();
	blobj->set_rotation(Vector3(t / 2.0, 0, 0));
	blobj->rotate(Vector3(0, t / 2.0, 0));

	gfx_prog->set_parameter("foo", foo);
	blobj->set_highlight_line_width((1.0 - foo) * 8.0);

	//sf_flat->triangulate(&blob_art->mesh, iso_val, t, true);
	//blob_art->mesh.invert_winding();
	
	scene->render(time);
	//blobj->render(time);
}


static void update_blobs(unsigned long time) {
	for(int i=0; i<bcount; i++) {
		blob[i].pos = blob[i].xflink->get_position(time);
	}
}

static float eval_func(const Vector3 &vec, float t) {
	float val = 0.0;
	for(int i=0; i<bcount; i++) {
		val += blob[i].energy / (vec - blob[i].pos).length_sq();
	}

	val += 1.0 / SQ(vec.x - wall_dist);
	val += 1.0 / SQ(vec.x + wall_dist);
	val += 1.0 / SQ(vec.y - wall_dist);
	val += 1.0 / SQ(vec.y + wall_dist);
	val += 1.0 / SQ(vec.z - wall_dist);
	val += 1.0 / SQ(vec.z + wall_dist);
	
	return val;
}

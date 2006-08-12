#include "p_cyl.h"

static Object *cyl;
static GfxProg *prog;

Vertex *orig_verts;

CylPart::CylPart() : dsys::ScenePart("cyl", (Scene*)0) {
	scene = new Scene;
	scene->set_auto_clear(false);
	scene->add_camera(new TargetCamera(Vector3(0, 0, -65)));
	scene->add_light(new PointLight(Vector3(-10, 0, -100)));

	cyl = new ObjCylinder(10.0, 65.0, false, 40, 40);
	cyl->mat.diffuse_color = 1.0;
	cyl->mat.specular_color = 1.0;
	cyl->mat.specular_power = 60.0;

	Texture *tex = get_texture("data/img/metal.jpg");
	Texture *env = get_texture("data/img/refmap2.png");
	if(!tex || !env) exit(0);

	cyl->mat.set_texture(tex, TEXTYPE_DIFFUSE);
	cyl->mat.set_texture(env, TEXTYPE_ENVMAP);
	scene->add_object(cyl);

	unsigned int vcount = cyl->get_mesh_ptr()->get_vertex_array()->get_count();
	orig_verts = new Vertex[vcount];
	memcpy(orig_verts, cyl->get_mesh_ptr()->get_vertex_array()->get_data(), vcount * sizeof(Vertex));

	prog = new GfxProg(get_shader("sdr/cyl_v.glsl", PROG_VERTEX), get_shader("sdr/cyl_p.glsl", PROG_PIXEL));
	prog->link();
	if(!prog->is_linked()) {
		exit(0);
	}

	cyl->set_gfx_program(prog);

}

CylPart::~CylPart() {
	delete [] orig_verts;
}

#define psin(x)		(sin(x) * 0.5 + 0.5)
#define pcos(x)		(cos(x) * 0.5 + 0.5)
#define MIN(a, b)	((a) < (b) ? (a) : (b))

void CylPart::draw_part() {
	float t = (float)time / 1000.0;


	unsigned int vcount = cyl->get_mesh_ptr()->get_vertex_array()->get_count();
	Vertex *vptr = cyl->get_mesh_ptr()->get_mod_vertex_array()->get_mod_data();

	for(int i=0; i<vcount; i++) {
		Vector3 v = orig_verts[i].pos;
		float y = (v.y + 32.5) * 1.5;
		
		float sfact = 0.3f + ((psin(sin(t) + cos(t/2)*y/8)+0.5f * pcos(t*2 + sin(y/5.0f)))/2);
		Vector3 vdir = v.normalized();
		sfact *= psin( acos(dot_product(vdir, Vector3(0, 0, 1)))*(2.5f+psin(t)*2.0f)/*4.0f*/ ) + 1.0f;

		float rfact = (sin(t/3) + cos(y*sin(t/2)/6) + cos(t) + cos(y*sin(t/4)/3) + cos(t/2)) * 0.4f;

		Matrix4x4 rotate, scale;
		scale.set_scaling(Vector3(sfact, 1, sfact));
		rotate.set_rotation(Vector3(0, rfact, 0));

		(vptr++)->pos = v.transformed(scale * rotate);
	}
	cyl->calculate_normals();
	
	const float in_dur = 3.0;
	float cyl_x = sin(MIN(half_pi * t / in_dur, half_pi)) * 85.0;
	cyl->set_position(Vector3(cyl_x - 60.0, 0, 0));

	glext::glActiveTexture(GL_TEXTURE0);
	set_texture(0, get_texture("data/img/metal.jpg"));
	prog->set_parameter("tex_dif", 0);
	glext::glActiveTexture(GL_TEXTURE1);
	set_texture(1, get_texture("data/img/refmap2.png"));
	prog->set_parameter("tex_env", 1);

	scene->render(0);

	glext::glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glext::glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

}

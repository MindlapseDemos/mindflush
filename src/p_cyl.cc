#include "p_cyl.h"

static Object *cyl;
static GfxProg *cyl_prog;

Vertex *orig_verts;

CylPart::CylPart() : dsys::ScenePart("cyl", (Scene*)0) {
	scene = new Scene;
	scene->set_auto_clear(false);
	scene->add_camera(new TargetCamera(Vector3(0, 0, -65)));
	scene->add_light(new PointLight(Vector3(-10, 0, -100)));

	cyl = new ObjCylinder(10.0, 65.0, false, 40, 40);
	cyl->set_position(Vector3(0, 0, 0));
	scene->add_object(cyl);

	unsigned int vcount = cyl->get_mesh_ptr()->get_vertex_array()->get_count();
	orig_verts = new Vertex[vcount];
	memcpy(orig_verts, cyl->get_mesh_ptr()->get_vertex_array()->get_data(), vcount * sizeof(Vertex));

	//cyl_prog = new GfxProg(get_shader("sdr/cyl_v.glsl", PROG_VERTEX), 0);
	//cyl->set_gfx_program(cyl_prog);
}

CylPart::~CylPart() {
	delete [] orig_verts;
}

#define psin(x)		(sin(x) * 0.5 + 0.5)
#define pcos(x)		(cos(x) * 0.5 + 0.5)

void CylPart::draw_part() {
	float t = (float)time / 1000.0;

	unsigned int vcount = cyl->get_mesh_ptr()->get_vertex_array()->get_count();
	Vertex *vptr = cyl->get_mesh_ptr()->get_mod_vertex_array()->get_mod_data();

	t *= 2.0;

	for(int i=0; i<vcount; i++) {
		Vector3 v = orig_verts[i].pos;
		float y = v.y;
		
		float sfact = 0.3f + ((psin(sin(t) + cos(t/2)*y/8)+0.5f * pcos(t*2 + sin(y/5.0f)))/2);
		Vector3 vdir = v.normalized();
		sfact *= psin( acos(dot_product(vdir, Vector3(0, 0, 1)))*(2.5f+psin(t)*2.0f)/*4.0f*/ ) + 1.0f;

		float rfact = (sin(t/3) + cos(y*sin(t/2)/6) + cos(t) + cos(y*sin(t/4)/3) + cos(t/2)) * 0.4f;

		Matrix4x4 rotate, scale;
		scale.set_scaling(Vector3(sfact, 1, sfact));
		rotate.set_rotation(Vector3(0, rfact, 0));

		(vptr++)->pos = v.transformed(scale * rotate);
	}

	scene->render(0);
}

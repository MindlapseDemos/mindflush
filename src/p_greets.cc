#include "p_greets.h"

static void make_skycube(Scene *scene);

static TargetCamera *cam;
static XFormNode *targ;

GreetsPart::GreetsPart() : dsys::ScenePart("greets", "data/geom/flygreets.3ds") {
	make_skycube(scene);

	cam = (TargetCamera*)scene->get_active_camera();
	cam->set_clipping_plane(50000, CLIP_FAR);
	cam->set_position(Vector3(0, 0, 0));
	cam->target.set_position(Vector3(0, 0, 0));

	Object *dummy = scene->get_object("Sphere01");
	scene->remove_object(dummy);

	dummy->children.push_back(targ);
	cam->target.parent = dummy;

	GfxProg *refr_prog = new GfxProg(get_shader("sdr/teapot_v.glsl", PROG_VERTEX), get_shader("sdr/teapot_p.glsl", PROG_PIXEL));
	refr_prog->link();
	if(!refr_prog->is_linked()) {
		exit(0);
	}

	std::list<Object*> *objlist = scene->get_object_list();
	std::list<Object*>::iterator iter = objlist->begin();
	while(iter != objlist->end()) {
		Object *obj = *iter++;
		if(obj->mat.name == "03 - Default") {
			obj->set_gfx_program(refr_prog);
		}
	}
}

GreetsPart::~GreetsPart() {
}

void GreetsPart::draw_part() {
	scene->render(time / 2);
	//scene->render(6170);
}

static void make_skycube(Scene *scene) {
	const float size = 10000;
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
		mat->diffuse_color = 0.0;
		add_texture(tex[i]);
		mat->set_texture(tex[i], TEXTYPE_DIFFUSE);
		face[i]->set_texture_addressing(TEXADDR_CLAMP);
		if(i != 1) scene->add_object(face[i]);
	}
}

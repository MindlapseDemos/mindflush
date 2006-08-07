#include <vector>
#include "p_plat.h"

using namespace std;
using namespace dsys;

static std::vector<RendCurve*> vines;

PlatPart::PlatPart() : ScenePart("plat", "data/geom/6plat.3ds") {
	Curve *spline;
	int i = 1;
	do {
		char name[16];
		sprintf(name, "Vine%2d", i++);
		if((spline = scene->get_curve(name))) {
			RendCurve *rcurve = new RendCurve(spline);
			vines.push_back(rcurve);
		}
	} while(spline && i < 100);
}

PlatPart::~PlatPart() {
	for(size_t i=0; i<vines.size(); i++) {
		delete vines[i];
	}
}

void PlatPart::draw_part() {
	for(size_t i=0; i<vines.size(); i++) {
		vines[i]->render();
	}

	scene->render(time);
}

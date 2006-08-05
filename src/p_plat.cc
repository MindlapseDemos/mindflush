#include "p_plat.h"

using namespace dsys;

PlatPart::PlatPart() : ScenePart("plat", "data/geom/6plat.3ds") {
}

PlatPart::~PlatPart() {
}

void PlatPart::draw_part() {
	scene->render(time);
}

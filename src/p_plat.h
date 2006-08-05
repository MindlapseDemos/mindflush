#ifndef _PLAT_H_
#define _PLAT_H_

#include "3dengfx/3dengfx.hpp"
#include "dsys/demosys.hpp"

class PlatPart : public dsys::ScenePart {
private:
	virtual void draw_part();

public:
	PlatPart();
	virtual ~PlatPart();
};

#endif	// _PLAT_H_

#ifndef _INTRO_H_
#define _INTRO_H_

#include "3dengfx/3dengfx.hpp"
#include "dsys/demosys.hpp"

class IntroPart : public dsys::ScenePart {
private:
	virtual void draw_part();

public:
	IntroPart();
	virtual ~IntroPart();
};

#endif	// _INTRO_H_

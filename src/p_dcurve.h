#ifndef _DCURVE_H_
#define _DCURVE_H_

#include "3dengfx/3dengfx.hpp"
#include "dsys/demosys.hpp"

class DCurvePart : public dsys::ScenePart {
private:
	virtual void draw_part();

public:
	DCurvePart();
	virtual ~DCurvePart();
};

#endif	// _DCURVE_H_

#ifndef _CREDITS_H_
#define _CREDITS_H_

#include "3dengfx/3dengfx.hpp"
#include "dsys/demosys.hpp"

class CreditsPart : public dsys::ScenePart {
private:
	virtual void draw_part();

public:
	CreditsPart();
	virtual ~CreditsPart();
};

#endif	// _CREDITS_H_

#ifndef _P_GREETS_H_
#define _P_GREETS_H_

#include "3dengfx/3dengfx.hpp"
#include "dsys/demosys.hpp"

class GreetsPart : public dsys::ScenePart {
private:
	virtual void draw_part();

public:
	GreetsPart();
	virtual ~GreetsPart();
};


#endif	/* _P_GREETS_H_ */

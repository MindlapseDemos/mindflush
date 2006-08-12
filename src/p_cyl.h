#ifndef _P_CYL_H_
#define _P_CYL_H_

#include "3dengfx/3dengfx.hpp"
#include "dsys/demosys.hpp"

class CylPart : public dsys::ScenePart {
private:
	virtual void draw_part();

public:
	CylPart();
	virtual ~CylPart();
};

#endif	/* _P_CYL_H_ */

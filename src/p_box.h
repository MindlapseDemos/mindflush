#ifndef P_BOX_H_
#define P_BOX_H_

#include "3dengfx/3dengfx.hpp"
#include "dsys/demosys.hpp"

class BoxPart : public dsys::ScenePart {
private:
	virtual void draw_part();

public:
	BoxPart();
	virtual ~BoxPart();
};

#endif	// P_BOX_H_

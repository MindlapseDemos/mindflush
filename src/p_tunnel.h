#ifndef _TUNNEL_H_
#define _TUNNEL_H_

#include "3dengfx/3dengfx.hpp"
#include "dsys/demosys.hpp"

class TunnelPart : public dsys::ScenePart {
private:
	virtual void draw_part();

public:
	TunnelPart();
	virtual ~TunnelPart();
};

#endif	// _TUNNEL_H_

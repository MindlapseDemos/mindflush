#ifndef _DEMO_PARTS_H_
#define _DEMO_PARTS_H_

#include "p_box.h"
#include "p_plat.h"
#include "p_intro.h"
#include "p_tunnel.h"
#include "p_dcurve.h"
#include "p_cyl.h"
#include "p_greets.h"

#define ADD_ALL_PARTS() {\
	parts.push_back(new BoxPart);\
	parts.push_back(new PlatPart);\
	parts.push_back(new IntroPart);\
	parts.push_back(new TunnelPart);\
	parts.push_back(new DCurvePart);\
	parts.push_back(new CylPart);\
	parts.push_back(new GreetsPart);\
}

#endif	// _DEMO_PARTS_H_

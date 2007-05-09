//#define PART_INTRO
//#define PART_PLAT
//#define PART_TUNNEL
//#define PART_BOX
//#define PART_CURVE
//#define PART_GREETS
#define PART_CREDITS

#define SINGLE_PART

#ifdef SINGLE_PART
#define START(x)	0
#else
#define START(x)	x
#endif

#ifdef PART_CURVE
0s		rename_part	dcurve	zdcurve
#endif
#ifdef PART_TUNNEL
0s		rename_part tunnel	0tunnel
#endif

#ifdef PART_INTRO
0 fx overlay 3500 3500 t0 sdr/radial_p.glsl
0 fx overlay 7250 3500 t0 sdr/radial_p.glsl
// hal speech radial
0 fx overlay 11000 1500 data/img/haltext1.png sdr/radial3_p.glsl
0 fx overlay 12500 1100 data/img/haltext2.png sdr/radial3_p.glsl
0 fx overlay 13600 900 data/img/haltext3.png sdr/radial3_p.glsl
// mindflush logo distortion
0 fx overlay 14500 7500 data/img/logo.jpg sdr/disp2_p.glsl
#endif
#ifdef PART_TUNNEL
// tunnel radial blur
0 fx overlay 42s 11s t0 sdr/radial2_p.glsl
#endif

#ifdef PART_INTRO
START(0)	start_part	intro
0			set_rtarget	intro t0
10750		end_part	intro
#endif

#ifdef PART_PLAT
START(22s)		start_part	plat
#endif

#ifdef PART_TUNNEL
START(40s)		start_part	0tunnel
#endif

#ifdef PART_PLAT
42s			end_part	plat
#endif

#ifdef PART_TUNNEL
42s			set_rtarget 0tunnel t0
#endif
#ifdef PART_BOX
START(44s)	start_part	box
44s			set_rtarget box t0
53s			set_rtarget box fb
#endif
#ifdef PART_TUNNEL
55s			end_part	0tunnel
#endif
#ifdef PART_CURVE
START(58s)	start_part	zdcurve
68s			end_part 	zdcurve
#endif
#ifdef PART_BOX
70s			end_part	box
#endif

#ifdef PART_GREETS
START(70s)	start_part	greets
90s			end_part	greets
#endif

#ifdef PART_CREDITS
START(90s)	start_part credits
190s		end_part credits
#endif

/* This is the demoscript source.
 * Pass through the C preprocessor and pipe through sort -n before feeding to 3dengfx.
 * (see tools/genscript).
 */
#define PART_INTRO
#define PART_PLAT
#define PART_TUNNEL
#define PART_BOX
#define PART_CURVE
#define PART_GREETS
#define PART_CREDITS

//#define SINGLE_PART

#ifdef SINGLE_PART
#define START(x)	0
#else
#define START(x)	x
#endif

/* ----- intro part ----- */
#ifdef PART_INTRO
0 fx overlay 3500 3500 t0 sdr/radial_p.glsl
0 fx overlay 7250 3500 t0 sdr/radial_p.glsl
// hal speech radial
0 fx overlay 11000 1500 data/img/haltext1.png sdr/radial3_p.glsl
0 fx overlay 12500 1100 data/img/haltext2.png sdr/radial3_p.glsl
0 fx overlay 13600 900 data/img/haltext3.png sdr/radial3_p.glsl
// mindflush logo distortion
0 fx overlay 14500 7500 data/img/logo.jpg sdr/disp2_p.glsl
0 fx overlay 21500 1s data/img/null.ppm sdr/fade_unfade_p.glsl

START(0)		start_part	intro
0				set_rtarget	intro t0
10750			end_part	intro
#endif


/* ----- platonic solids part ----- */
#ifdef PART_PLAT
START(22000)	start_part	plat
42000			end_part	plat
#endif


/* ----- tunnel part ----- */
#ifdef PART_TUNNEL
// tunnel radial blur
0 fx overlay 42s 11s t0 sdr/radial2_p.glsl
0				rename_part tunnel	0tunnel

START(40000)	start_part	0tunnel
42000			set_rtarget 0tunnel t0
55000			end_part	0tunnel
#endif

#ifdef PART_BOX
0 fx overlay 69500 1s data/img/null.ppm sdr/fade_unfade_p.glsl

START(44000)	start_part	box
44000			set_rtarget box t0
53000			set_rtarget box fb
70000			end_part	box
#endif

#ifdef PART_CURVE
0				rename_part	dcurve	zdcurve
START(58000)	start_part	zdcurve
68000			end_part 	zdcurve
#endif

#ifdef PART_GREETS
// greetings to credits transition slide
0 fx overlay 88s 2s t1 sdr/slide_p.glsl
0		rename_part	greets	agreets

//0				start_part	agreets
//18000			set_rtarget agreets t1
//20000			end_part	agreets
START(70000)	start_part	agreets
88000			set_rtarget agreets t1
90000			end_part	agreets
#endif

#ifdef PART_CREDITS
0 fx overlay 128000 2s data/img/null.ppm sdr/fade2black_p.glsl

//18000			start_part credits
START(88000)	start_part credits
130000			end_part credits
#endif

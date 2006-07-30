#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

int main(int argc, char **argv) {
	float col_r = 1.0;
	float col_g = 1.0;
	float col_b = 1.0;
	int i, x, y, ysz, xsz = 256;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-' && argv[i][2] == 0) {
			switch(argv[i][1]) {
			case 's':
				if(!isdigit(argv[++i][0])) {
					fprintf(stderr, "-s must be followed by a number (size)\n");
					return EXIT_FAILURE;
				}
				xsz = atoi(argv[i]);
				break;

			case 'c':
				{
					char *r, *g, *b;
					r = argv[++i];
					g = strchr(r + 1, ',');
					b = g ? strchr(g + 1, ',') : 0;

					if(!isdigit(*r) || !g || !isdigit(*++g) || !b || !isdigit(*++b)) {
						fprintf(stderr, "-c must be followed by a triplet r, g, b\n");
						return EXIT_FAILURE;
					}
					col_r = atof(r);
					col_g = atof(g);
					col_b = atof(b);
				}
				break;

			default:
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				return EXIT_FAILURE;
			}
		} else {
			fprintf(stderr, "unexpected argument: %s\n", argv[i]);
			return EXIT_FAILURE;
		}
	}

	ysz = xsz / 2;
	
	printf("P6\n%d %d\n255\n", xsz, ysz);
	for(y=0; y<ysz; y++) {
		float r, g, b, dr, dg, db;
		int nth_inc = y + 1;
		r = g = b = 0.0;
		dr = dg = db = (float)nth_inc / (float)xsz;
		for(x=0; x<xsz; x++) {
			putchar((char)(r * 255.0));
			putchar((char)(g * 255.0));
			putchar((char)(b * 255.0));

			/*if(x == xsz - 1) {
				fprintf(stderr, "ln(%d).end -> %d\n", y, (int)(r * 255.0));
			}*/

			if(x > 0 && x % nth_inc == 0) {
				r += dr;
				g += dg;
				b += db;
			}
		}
	}

	return 0;
}

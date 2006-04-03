uniform sampler2D tex;

void main() {
	const int iter = 40;
	vec2 z, c;

	c.x = 1.3333 * (gl_TexCoord[0].x - 0.7) / 0.5;
	c.y = (gl_TexCoord[0].y - 0.5) / 0.5;
	c *= 1.1;

	int i;
	z = c;
	for(i=0; i<iter; i++) {
		float x = (z.x * z.x - z.y * z.y) + c.x;
		float y = (z.y * z.x + z.x * z.y) + c.y;

		if((x * x + y * y) > 4.0) break;
		z.x = x;
		z.y = y;
	}

	gl_FragColor = texture2D(tex, vec2((float)(i == iter ? 0 : i) / 100, 0));
}

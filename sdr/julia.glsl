uniform sampler2D tex;
uniform float t;

void main() {
	const int iter = 40;
	vec2 z, z0, c;
	z0.x = 1.5 * (gl_TexCoord[0].x - 0.5) / 0.5;
	z0.y = (gl_TexCoord[0].y - 0.5) / 0.5;

	t *= 80.0;
	c.x = (sin(cos(t / 10) * 10) + cos(t * 2.0) / 4.0 + sin(t * 3.0) / 6.0) * 0.8;
	c.y = (cos(sin(t / 10) * 10) + sin(t * 2.0) / 4.0 + cos(t * 3.0) / 6.0) * 0.8;

	int i;
	
	z = z0;
	for(i=0; i<iter; i++) {
		float x = (z.x * z.x - z.y * z.y) + c.x;
		float y = (z.y * z.x + z.x * z.y) + c.y;

		if((x * x + y * y) > 4.0) break;
		z.x = x;
		z.y = y;
	}

	gl_FragColor = texture2D(tex, vec2((float)(i == iter ? 0 : i) / 100, 0));
}

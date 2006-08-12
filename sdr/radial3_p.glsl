uniform sampler2D tex;
uniform float time;
uniform float ease_sin;

#define TC		gl_TexCoord[0].st

void main() {
	const vec2 center = vec2(0.5, 0.5);
	const int samples = 10;
	const float fsamples = float(samples);

	vec4 col = vec4(0, 0, 0, 0);//texture2D(tex, TC);

	for(int i=0; i<samples; i++) {
		float fi = float(i);
		vec2 dir = (TC - center);
		col += texture2D(tex, TC + fi * 0.01 * ease_sin * -dir) * (1.0 - fi / fsamples);
	}

	col *= 0.3;

	gl_FragColor = gl_Color * col;
}

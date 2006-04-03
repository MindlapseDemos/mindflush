uniform float time;
uniform float t;
uniform float ease;
uniform float ease_sin;

uniform sampler2D tex;

#define PI	3.14158

#define F0	6.0
#define F1	8.0
#define F2	10.0
#define A0	1.0
#define A1	2.0
#define A2	4.0

#define X	gl_TexCoord[0].s
#define Y	gl_TexCoord[0].t

void main() {
	float sint = sin(time);
	float cost = cos(time);

	float dx = 	sin((X + cost) * F0) / A0 +
				cos((X + sint) * F1) / A1 +
				sin((X + cost) * F2) / A2;

	float dy = 	cos((Y + sint) * F0) / A0 +
				sin((Y + cost) * F1) / A1 +
				cos((Y + sint) * F2) / A2;

	float tex_x = clamp(X + ease_sin * dx / 16.0, 0.0, 1.0);
	float tex_y = clamp(Y + ease_sin * dy / 16.0, 0.0, 1.0);
	
	vec4 tex_col = texture2D(tex, vec2(tex_x, tex_y));
	gl_FragColor = gl_Color * tex_col;
}

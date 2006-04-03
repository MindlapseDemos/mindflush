uniform float t;
uniform sampler2D tex;

#define PI	3.14158
#define X	gl_TexCoord[0].s
#define Y	gl_TexCoord[0].t

void main() {
	float sep = sin(Y * PI * 4.0) / 2.0 + 0.5;
	bool left = sep < 0.5;

	vec2 tc = gl_TexCoord[0].st;
	if(left) {
		if(X < t) discard;
		tc.x -= t;
	} else {
		if((1.0 - X) < t) discard;
		tc.x += t;
	}
		
	gl_FragColor = gl_Color * texture2D(tex, tc);
}

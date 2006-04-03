uniform float t;

uniform sampler2D tex;

#define X	gl_TexCoord[0].s
#define Y	gl_TexCoord[0].t

void main() {
	float tex_x = min(X, t * 2.0);
	vec4 tex_col = texture2D(tex, vec2(tex_x, Y));
	gl_FragColor = gl_Color * tex_col;
}

uniform sampler2D text, text_blur;
uniform float glow_param;
uniform float t;

void main()
{
	vec2 tc = gl_TexCoord[0].yx;
	tc.y = max(0.0, min(tc.y * 3.0 - t / 40.0 - 0.5, 1.0));

	vec4 tcol = texture2D(text, tc) * 0.5;
	vec4 bcol = texture2D(text_blur, tc) * glow_param * 1.2;
	
	gl_FragColor = tcol + bcol;
}

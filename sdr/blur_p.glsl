uniform sampler2D tex;

void main()
{
	vec4 diff = gl_FrontMaterial.diffuse;
	vec2 tc0 = gl_TexCoord[0].st;

	float offset = 0.004;
	vec4 sample = texture2D(tex, tc0);
	sample += texture2D(tex, tc0 + vec2(offset, 0.0));
	sample += texture2D(tex, tc0 - vec2(offset, 0.0));
	sample += texture2D(tex, tc0 + vec2(0.0, offset));
	sample += texture2D(tex, tc0 - vec2(0.0, offset));

	sample += texture2D(tex, tc0 + vec2(offset, offset));
	sample += texture2D(tex, tc0 + vec2(-offset, offset));
	sample += texture2D(tex, tc0 + vec2(offset, -offset));
	sample += texture2D(tex, tc0 + vec2(-offset, -offset));

	sample += texture2D(tex, tc0 + vec2(offset * 2.0, 0.0));
	sample += texture2D(tex, tc0 - vec2(offset * 2.0, 0.0));
	sample += texture2D(tex, tc0 + vec2(0.0, offset * 2.0));
	sample += texture2D(tex, tc0 - vec2(0.0, offset * 2.0));

	sample += texture2D(tex, tc0 + vec2(offset * 3.0, 0.0));
	sample += texture2D(tex, tc0 - vec2(offset * 3.0, 0.0));
	sample += texture2D(tex, tc0 + vec2(0.0, offset * 3.0));
	sample += texture2D(tex, tc0 - vec2(0.0, offset * 3.0));
	
	gl_FragColor = diff * (sample / 17.0);
}

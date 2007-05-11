uniform float compl;

#define H	0.05

void main()
{
	vec4 col = gl_FrontMaterial.diffuse;
	vec2 tc = gl_TexCoord[0].st;
	float alpha = 1.0 - smoothstep(compl - H, compl + H, tc.t);
	gl_FragColor = col * alpha * col.a;
}

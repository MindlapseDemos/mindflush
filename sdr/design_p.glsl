uniform sampler2D tex;
uniform float compl;

void main()
{
	vec4 tcol = texture2D(tex, gl_TexCoord[0].st);
	tcol.a = tcol.r < compl && tcol.r > 0.01 ? 1.0 : 0.0;//smoothstep(compl - 0.1, compl + 0.1, tcol.a);
	gl_FragColor = tcol;
}

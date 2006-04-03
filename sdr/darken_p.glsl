uniform sampler2D tex;

void main() {
	vec4 texel = texture2D(tex, gl_TexCoord[0].st);
	gl_FragColor = vec4(0.8, 0.2, 0.1, 1) * gl_Color * texel; 
}

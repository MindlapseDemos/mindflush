uniform sampler2D tex;
varying vec3 vcs_pos;
varying vec3 normal;

void main() {
	vec4 texcol = texture2D(tex, gl_TexCoord[0].st);

	vec3 l = normalize(gl_LightSource[0].position.xyz - vcs_pos);
	vec3 n = normalize(normal + (texcol.xyz - vec3(0.5, 0.5, 0.5)));
	vec3 v = vec3(0.0, 0.0, -1.0);//-normalize(vcs_pos);
	vec3 h = normalize(l + v);

	float idif = max(dot(l, n), 0.0);
	float ispec = pow(max(dot(h, n), 0.0), gl_FrontMaterial.shininess);

	vec4 color = texture2D(tex, gl_TexCoord[0].st) * idif + gl_FrontMaterial.specular * ispec;
	color.a = gl_FrontMaterial.diffuse.a;

	gl_FragColor = color;
}

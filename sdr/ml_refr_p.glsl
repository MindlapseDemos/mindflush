varying vec3 ldir;
varying vec3 vcs_pos;

uniform sampler2D nmap;

void main() {
	vec3 vdir = -normalize(vcs_pos);
	vec3 half_vec = (ldir + vdir) / 2.0;

	vec3 normal = (texture2D(nmap, gl_TexCoord[0].st).rgb - vec3(0.5, 0.5, 0.5)) * 2.0;

	float idif = max(dot(ldir, normal), 0.0);
	float ispec = pow(max(dot(half_vec, normal), 0.0), 20.0);

	vec4 diffuse = gl_FrontMaterial.diffuse * idif;
	vec4 specular = gl_FrontMaterial.specular * ispec;

	gl_FragColor = diffuse + specular;
}

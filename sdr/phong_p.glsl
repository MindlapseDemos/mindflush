uniform sampler2D cel_tex;
uniform float foo;
varying vec3 vcs_pos;
varying vec3 normal;

void main() {
	vec3 l = normalize(gl_LightSource[0].position.xyz - vcs_pos);
	vec3 n = normalize(normal);
	vec3 v = vec3(0.0, 0.0, -1.0);//-normalize(vcs_pos);
	vec3 h = normalize(l + v);

	float idif = max(dot(l, n), 0.0);
	float ispec = pow(max(dot(h, n), 0.0), gl_FrontMaterial.shininess);

	vec4 color = texture2D(cel_tex, vec2(idif, foo)) * gl_FrontMaterial.diffuse;
	if(ispec > min(1.0 - foo, 0.9)) {
		color += ispec * gl_FrontMaterial.specular;
	}
	color.a = gl_FrontMaterial.diffuse.a;

	gl_FragColor = color;
}

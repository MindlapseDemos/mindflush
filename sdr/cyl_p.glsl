uniform sampler2D tex_dif, tex_env;

varying vec3 vcs_pos, normal;

vec2 sphere_map(vec3 normal) {
	const vec3 vdir = vec3(0.0, 0.0, -1.0);
	vec3 refl = reflect(vdir, normal);
	float m = 2.0 * sqrt(refl.x * refl.x + refl.y * refl.y + (refl.z+1.0) * (refl.z+1.0));
	return vec2(-refl.x / m, -refl.y / m) + vec2(0.5, 0.5);
}

void main() {
	vec3 l = normalize(gl_LightSource[0].position.xyz - vcs_pos);
	vec3 n = normalize(normal);
	vec3 v = vec3(0.0, 0.0, -1.0);//-normalize(vcs_pos);
	vec3 h = normalize(l + v);

	float idif = max(dot(l, n), 0.0);
	float ispec = pow(max(dot(h, n), 0.0), gl_FrontMaterial.shininess);

	vec2 env_tc = sphere_map(n);

	vec4 texel_dif = texture2D(tex_dif, gl_TexCoord[0].st);
	vec4 texel_env = texture2D(tex_env, env_tc);

	vec4 color = texel_dif * idif + gl_FrontMaterial.specular * ispec + texel_env;
	gl_FragColor = color;
}

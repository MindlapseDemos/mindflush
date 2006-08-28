float fast_fresnel(vec3 i, vec3 n, vec3 fresnel_val) {
	return fresnel_val.z + pow(1.0 - dot(i, n), fresnel_val.x) * fresnel_val.y;
}

vec4 calc_lighting(vec3 normal, vec3 ldir) {
	const vec3 vdir = vec3(0.0, 0.0, -1.0);
	vec3 hvec = (ldir + vdir) / 2.0;

	float d = max(dot(normal, ldir), 0.0);
	float s = pow(max(dot(normal, hvec), 0.0), gl_FrontMaterial.shininess);
	
	return d * gl_FrontMaterial.diffuse + s * gl_FrontMaterial.specular;
}

varying float fresnel;
varying vec3 refl_vec, refr_vec;

void main(void) {
	gl_Position = ftransform();

	vec3 pos_vcs = vec3(gl_ModelViewMatrix * gl_Vertex);
	vec3 view_vec = normalize(pos_vcs);

	vec3 normal = gl_NormalMatrix * gl_Normal;
	normal = normalize(normal);

	refl_vec = reflect(view_vec, normal);
	refl_vec = (gl_TextureMatrix[0] * vec4(refl_vec, 1.0)).xyz;

	const float theta = 1.05;
	refr_vec = refract(view_vec, normal, theta);
	if(refr_vec.x == 0.0 && refr_vec.y == 0.0 && refr_vec.z == 0.0) {
		refr_vec = refl_vec;
	}
	refr_vec = (gl_TextureMatrix[0] * vec4(refr_vec, 1.0)).xyz;

	vec3 lpos = gl_LightSource[0].position.xyz;
	vec3 ldir = normalize(lpos - pos_vcs);

	gl_FrontColor = calc_lighting(normal, ldir);

	fresnel = fast_fresnel(-view_vec, normal, vec3(2.0, 1.0, 0.0));
}

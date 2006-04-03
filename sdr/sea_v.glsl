uniform float t;
uniform float hscale;

void main() {
	const vec3 waves_x = vec3(1.0, 0.5, 0.3) * 0.1;
	const vec3 waves_y = vec3(0.5, 1.0, 0.0) * 0.1;
	vec3 waves_h = vec3(2.0, 1.5, 1.0) * hscale;
	vec2 pos = gl_Vertex.xz * 0.3;

	vec3 angle = waves_x * pos.x + waves_y * pos.y + t;

	vec3 sine = sin(angle);
	vec3 cosine = cos(angle);

	vec4 out_pos = ftransform();
	out_pos.y += dot(waves_h, sine);
	gl_Position = out_pos;

	vec3 normal;
	normal.x = dot(waves_h * waves_x, cosine);
	normal.y = 1.0;
	normal.z = dot(waves_h * waves_y, cosine);
	normal = normalize(normal);

	// transform position to view space for lighting
	vec3 pos_vcs = (gl_ModelViewMatrix * gl_Vertex).xyz;
	vec3 view_vec = normalize(pos_vcs);

	gl_FrontColor = vec4(0, 0, 0, 1);

	vec4 refl_vec = reflect(view_vec, normal).xyzz;
	refl_vec.w = 1.0;
	gl_TexCoord[0] = gl_TextureMatrix[0] * refl_vec;
	//gl_TexCoord[0].w = 1.0;
}

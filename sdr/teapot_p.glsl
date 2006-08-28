varying float fresnel;
varying vec3 refl_vec, refr_vec;

uniform samplerCube cube_tex;

vec4 lerp4(vec4 a, vec4 b, float t) {
	return a + (b - a) * t;
}

void main(void) {
	vec4 refr_col = textureCube(cube_tex, refr_vec);
	vec4 refl_col = textureCube(cube_tex, refl_vec);

	vec4 col = lerp4(refr_col, refl_col, fresnel);
	gl_FragColor = gl_Color + col * vec4(0.6, 0.75, 1.0, 1.0);
}

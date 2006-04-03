varying vec3 vcs_pos;
varying vec3 normal;

void main() {
	vec3 l = normalize(gl_LightSource[0].position.xyz - vcs_pos);
	vec3 n = normalize(normal);
	vec3 v = -normalize(vcs_pos);
	vec3 h = normalize(l + v);

	float idif = abs(dot(l, n));
	float ispec = pow(abs(dot(h, n)), gl_FrontMaterial.shininess);

	vec4 diffuse = gl_FrontMaterial.diffuse * idif;
	vec4 specular = gl_FrontMaterial.specular * ispec;

	// uncomment this for quantization
	//vec4 color = ispec > 0.8 ? gl_FrontMaterial.specular : (diffuse > 0.5 ? diffuse : gl_LightModel.ambient);
	vec4 color = gl_LightModel.ambient + diffuse + specular;
	color.a = diffuse.a;

	gl_FragColor = color;
}

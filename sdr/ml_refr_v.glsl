varying vec3 ldir;
varying vec3 vcs_pos;

void main() {
	vec3 tangent = vec3(1.0, 0.0, 0.0) * gl_NormalMatrix;
	vec3 bitangent = vec3(0.0, 1.0, 0.0) * gl_NormalMatrix;
	vec3 normal = cross(bitangent, tangent);
	//vec3 normal = cross(tangent, bitangent);
	//vec4 foo = vec4(0.0, 0.0, 0.0, 1.0);

	//mat3 mat = mat3(tangent, bitangent, normal);
	//mat3 mat = mat3(tangent.x, tangent.y, tangent.z,
	//				bitangent.x, bitangent.y, bitangent.z,
	//				normal.x, normal.y, normal.z);
	mat3 mat = mat3(tangent.x, bitangent.x, normal.x,
					tangent.y, bitangent.y, normal.y,
					tangent.z, bitangent.z, normal.z);

	vcs_pos = (gl_Vertex * gl_ModelViewMatrix).xyz;
	
	vec4 lpos = gl_LightSource[0].position;
	ldir = normalize(lpos.xyz - vcs_pos) * mat;

	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;
}

varying vec3 vcs_pos, normal;

void main() {
	vcs_pos = (gl_ModelViewMatrix * gl_Vertex).xyz;
	normal = (gl_NormalMatrix * gl_Normal).xyz;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}

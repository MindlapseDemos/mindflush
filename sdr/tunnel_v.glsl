varying float depth;

void main() {
	depth = (gl_ModelViewMatrix * gl_Vertex).z;
	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;
}

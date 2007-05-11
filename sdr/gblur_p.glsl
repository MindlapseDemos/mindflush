/* Gaussian Blur */
/*
almost gaussian kernel:
0       8       10      8       0
8       17      21      17      8
10      21      26      21      10
8       17      21      17      8
0       8       10      8       0
sum: 250
*/

uniform sampler2D tex;

void main()
{
	vec4 diff = gl_FrontMaterial.diffuse;
	vec2 tc = (gl_TextureMatrix[0] * gl_TexCoord[0]).st;
	float sum = 250.0;
	float offset = 0.005;

	vec4 sample = texture2D(tex, tc) * 26.0;

	sample += texture2D(tex, tc + vec2(offset, 0.0)) * 21.0;
	sample += texture2D(tex, tc + vec2(-offset, 0.0)) * 21.0;
	sample += texture2D(tex, tc + vec2(0.0, offset)) * 21.0;
	sample += texture2D(tex, tc + vec2(0.0, -offset)) * 21.0;
	
	sample += texture2D(tex, tc + vec2(offset, offset)) * 17.0;
	sample += texture2D(tex, tc + vec2(-offset, offset)) * 17.0;
	sample += texture2D(tex, tc + vec2(offset, -offset)) * 17.0;
	sample += texture2D(tex, tc + vec2(-offset, -offset)) * 17.0;
	
	float offs = offset * 2.0;
	sample += texture2D(tex, tc + vec2(offs, 0.0)) * 10.0;
	sample += texture2D(tex, tc + vec2(-offs, 0.0)) * 10.0;
	sample += texture2D(tex, tc + vec2(0.0, offs)) * 10.0;
	sample += texture2D(tex, tc + vec2(0.0, -offs)) * 10.0;

	sample += texture2D(tex, tc + vec2(offset, offs)) * 8.0;
	sample += texture2D(tex, tc + vec2(offset, -offs)) * 8.0;
	sample += texture2D(tex, tc + vec2(-offset, offs)) * 8.0;
	sample += texture2D(tex, tc + vec2(offset, -offs)) * 8.0;
	
	gl_FragColor = diff * (sample / sum);
}

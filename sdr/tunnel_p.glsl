// tunnel_p.glsl
varying float depth;

uniform float t;
uniform float farthest;
uniform sampler2D tex;

void main()
{
	vec2 tex_coord = gl_TexCoord[0].st;

	// convert to polar coordinates
	tex_coord.x = 2.0 * tex_coord.x - 1.0;
	tex_coord.y = 2.0 * tex_coord.y - 1.0;
	float r = length(tex_coord);
	float phi = atan2(tex_coord.y , tex_coord.x);

	//r += 0.1 * (sin(r * cos(t) * 10.0) + cos(r * sin(t) * 5.0) + sin(t * 3.0));

	r += 0.1 * sin(phi * 4.0);
	phi += sin(t);//sin(r * sin(t) * 4.0) * r;

	// convert back to cartesian space
	tex_coord.x = r * cos(phi);
	tex_coord.y = r * sin(phi);
	tex_coord.x = clamp((tex_coord.x + 1.0) / 2.0, 0.0, 1.0);
	tex_coord.y = clamp((tex_coord.y + 1.0) / 2.0, 0.0, 1.0);
	
	float sq = (depth / farthest) * (depth / farthest);
	gl_FragColor = texture2D(tex, tex_coord) * 1.0 - sq;//depth / farthest;
}


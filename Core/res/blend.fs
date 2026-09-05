uniform vec4 uColor;

#ifdef TEXTURE
uniform sampler2D uT;
varying vec2 vTC;
#endif

vec4 toSRGBA(vec4 linearRGBA)
{
	vec3 linearRGB = linearRGBA.rgb / linearRGBA.a;
	vec3 lower = linearRGB * 12.92;
	vec3 higher = 1.055 * pow(linearRGB, vec3(1.0 / 2.4)) - vec3(0.055);
	vec3 cutoff = step(vec3(0.0031308), linearRGB);
	vec4 sRGBA = linearRGBA.aaaa;
	sRGBA.rgb *= mix(lower, higher, cutoff);
	return sRGBA;
}

void main()
{
	vec4 color = uColor;

#ifdef TEXTURE
	color *= texture2D(uT, vTC);
#endif

#ifdef sRGB
	color = toSRGBA(color);
#endif

	gl_FragColor = color;
}

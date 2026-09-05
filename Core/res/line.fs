uniform vec4 uColor;

uniform vec2 uDash;
varying float vX;

void main()
{
	if (fract(vX * uDash.x + uDash.y) >= 0.5) discard;

	gl_FragColor = uColor;
}

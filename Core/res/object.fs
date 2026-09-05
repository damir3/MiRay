uniform vec4 uColor;

varying vec3 vNormal;

// vec3 lightDir = vec3(0.0, 0.0, 1.0);
vec3 lightDir = normalize(vec3(1.0, -2.0, 3.0));

void main()
{
	vec3 normal = normalize(vNormal);
	vec4 color = uColor;
	color.rgb *= dot(normal, lightDir) * 0.35 + 0.65;
	gl_FragColor = color;
}

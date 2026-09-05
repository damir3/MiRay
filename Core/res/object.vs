attribute vec4 aP;
attribute vec3 aN;
uniform mat4 uMVPM; // model view projection matrix
uniform mat4 uNM; // normal matrix

varying vec3 vNormal;

void main()
{
	gl_Position = uMVPM * aP;
	vNormal.x = dot(uNM[0].xyz, aN);
	vNormal.y = dot(uNM[1].xyz, aN);
	vNormal.z = dot(uNM[2].xyz, aN);
}

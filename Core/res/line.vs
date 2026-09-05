attribute vec4 aP;
uniform mat4 uMVPM; // model view projection matrix

varying float vX;

void main()
{
	gl_Position = uMVPM * aP;
	vX = aP.x;
}

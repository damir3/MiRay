attribute vec4 aP;
uniform mat4 uMVPM; // model view projection matrix

#ifdef TEXTURE
varying vec2 vTC;
uniform vec4 uTM;
#endif

void main()
{
	gl_Position = uMVPM * aP;
#ifdef TEXTURE
	vTC = aP.xy * uTM.xy + uTM.zw;
#endif
}

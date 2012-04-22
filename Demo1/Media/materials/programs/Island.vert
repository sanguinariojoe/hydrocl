// Island GLSL vertex program - For Hydrax demo application
// Jose Luis Cercós Pita

// In
// Out
varying vec3  LightDir;
varying vec3  EyeDir;
varying vec3  HalfAngle;
varying float YPos;
// Uniform
uniform vec4   uLightPosition; 
uniform vec3   uEyePosition;
uniform float  uTexturesScale;
uniform mat4   uWorldViewProj;	// Don't needed, only for HLSL/CG compatibility

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0*uTexturesScale;

	EyeDir   = normalize(uEyePosition - gl_Vertex.xyz);
	LightDir = normalize(uLightPosition.xyz -  (gl_Vertex.xyz * uLightPosition.w));

	HalfAngle = -normalize((gl_ModelViewProjectionMatrix * vec4(EyeDir + LightDir, 1.0)).xyz);

	YPos = gl_Vertex.y/200.0; //[0,1] range
}	


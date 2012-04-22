// Palm CG vertex program - For Hydrax demo application
// Jose Luis Cercós Pita

// In
// Out
varying vec3 Position;
varying vec3 Normal;
// Uniform
uniform float uTime;
uniform mat4  uWorld;
uniform mat4   uWorldViewProj;	// Don't needed, only for HLSL/CG compatibility

void main()
{
	vec4 Vertex = gl_Vertex;
	float Time = uTime; 
	if(gl_Vertex.z>2.0)
	{
		Time += ( uWorld * ( gl_Vertex - vec4(0.0,0.0,2.0,0.0) ) ).x * 0.01;
		Vertex.xz += vec2(sin(Time), cos(Time))*Vertex.z * 0.01;
	}

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	Position    = gl_Position.xyz;
	Normal      = normalize(gl_Normal);
	gl_TexCoord[0] = gl_MultiTexCoord0;
}


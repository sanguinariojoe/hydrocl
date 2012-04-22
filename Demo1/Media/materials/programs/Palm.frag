// Palm CG vertex program - For Hydrax demo application
// Jose Luis Cercós Pita

// In
varying vec3 Position;
varying vec3 Normal;
// Uniform
uniform vec3      uEyePosition;
uniform vec3      uLightPosition;
uniform vec3      uLightDiffuse;
uniform vec3      uLightSpecular;
uniform sampler2D uTexture;


void main()
{
    vec3 ViewVector = normalize(Position - uEyePosition);
    vec3 LightDir   = normalize(uLightPosition - Position);

    float Specular       = dot(Normal, normalize(ViewVector + LightDir));
	vec4  DiffTexture    = texture2D(uTexture, gl_TexCoord[0].xy);

	vec3 Final = DiffTexture.xyz*uLightDiffuse + uLightSpecular*Specular*0.5*clamp(2.0/Position.z, 0.0, 1.0);

	gl_FragColor = vec4(clamp(Final, 0.0, 1.0),DiffTexture.w);
}



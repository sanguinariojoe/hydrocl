// Island GLSL fragment program - For Hydrax demo application
// Jose Luis Cercós Pita

// In
varying vec3  LightDir;
varying vec3  EyeDir;
varying vec3  HalfAngle;
varying float YPos;
// Uniform
// Uniform
uniform vec3 uLightDiffuse;
uniform vec3 uLightSpecular;
uniform vec4 uScaleBias;
uniform sampler2D uNormalHeightMap1;
uniform sampler2D uDiffuseMap1;
uniform sampler2D uNormalHeightMap2;
uniform sampler2D uDiffuseMap2;

vec3 expand(vec3 v)
{
	return (v - 0.5) * 2.0;
}

vec3 calculate_colour(	sampler2D uNormalHeightMap,
						sampler2D uDiffuseMap,
						float _GreenFade)
{
    // get the height using the tex coords
	float height = texture2D(uNormalHeightMap, gl_TexCoord[0].xy).w;

	// calculate displacement
	float displacement = (height * uScaleBias.x) + uScaleBias.y;

	// calculate the new tex coord to use for normal and diffuse
	vec2 newTexCoord = ((EyeDir.xy * displacement) + gl_TexCoord[0].xy).xy;

	// get the new normal and diffuse values
	vec3 normal = expand(texture2D(uNormalHeightMap, newTexCoord).xyz);
	normal.z = 0.0;
	vec3 diffuse = texture2D(uDiffuseMap, newTexCoord).xyz;
	
	if (_GreenFade>0.6)
	{
	    float d = (_GreenFade-0.6)/3.0;
	    diffuse += vec3(-d,d,-d);
	}

	vec3 specular = pow(clamp(dot(normal, HalfAngle), 0.0, 1.0), 1.0) * uLightSpecular;
	float diff = clamp(dot(normal, LightDir), 0.0, 1.0);
	
	if (diff<0.5)
	{
	   diff = 0.5;
	}

	return diffuse * diff * uLightDiffuse + specular;
}

void main()
{
	vec3 col1 = calculate_colour(	uNormalHeightMap1, 
									uDiffuseMap1,
									0.0);
                                   
	vec3 col2 = calculate_colour(	uNormalHeightMap2, 
									uDiffuseMap2,
									YPos);
	
	gl_FragColor = vec4(mix(col1,col2,YPos), 1.0);
}



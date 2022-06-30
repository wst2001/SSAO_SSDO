#version 450 core
out vec3 FragColor;
in vec2 TexCoords;

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D ssdo;
uniform sampler2D ssdoBlur;
uniform sampler2D texLighting;
uniform sampler2D texIndirectLight;
uniform sampler2D texIndirectLightBlur;
uniform sampler2D texSkybox;

uniform int mode;

void main()
{
    // Get input for SSDO algorithm
    vec3 fragPos = texture(gPositionDepth, TexCoords).xyz;
    vec3 normal = texture(gNormal, TexCoords).rgb;	
	vec3 lighting = texture(texLighting, TexCoords).rgb;
	float depth = texture(gPositionDepth, TexCoords).a;
	vec3 directionalLight = texture(ssdo, TexCoords).rgb;
	vec3 directionalLightBlur = texture(ssdoBlur, TexCoords).rgb;
	vec3 indirectLight = texture(texIndirectLight, TexCoords).rgb;
	vec3 indirectLightBlur = texture(texIndirectLightBlur, TexCoords).rgb;
	vec3 skybox = texture(texSkybox, TexCoords).rgb;
	
    if (mode == 0)
        FragColor = normal;
    else if (mode == 1)
        FragColor = lighting;
	else if (mode == 2)
        FragColor = directionalLight;
	else if (mode == 3)
        FragColor = directionalLightBlur;
	else if (mode == 4)
        FragColor = indirectLight;
	else if (mode == 5)
        FragColor = indirectLightBlur;
    else if (mode == 6)
        FragColor = vec3( (depth-1) / 10 );
    else if (mode == 7)
        FragColor = skybox;
    else FragColor = ( depth != 1
        ?  lighting + directionalLightBlur + indirectLightBlur
        : skybox );
}

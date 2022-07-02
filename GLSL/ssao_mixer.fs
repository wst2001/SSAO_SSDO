#version 450 core
out vec3 FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D ssao;
uniform sampler2D ssaoBlur;
uniform sampler2D texLighting;
uniform sampler2D texSkybox;

uniform int mode;

void main()
{
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = texture(gNormal, TexCoords).rgb;	
	vec3 lighting = texture(texLighting, TexCoords).rgb;
	float depth = texture(gPosition, TexCoords).a;
	float ssaoOcclusion = texture(ssao, TexCoords).r;
	float ssaoOcclusionBlur = texture(ssaoBlur, TexCoords).r;
	vec3 skybox = texture(texSkybox, TexCoords).rgb;
	
    if (mode == 0)
        FragColor = normal;
    else if (mode == 1)
        FragColor = lighting;
	else if (mode == 2)
        FragColor = vec3(ssaoOcclusion);
	else if (mode == 3)
        FragColor = vec3(ssaoOcclusionBlur);
    else if (mode == 4)
        FragColor = vec3( (depth-1) / 10 );
    else if (mode == 5)
        FragColor = skybox;
    else FragColor = ( depth != 1 ?  lighting : skybox);
}

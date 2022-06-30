#version 450 core
out vec3 FragColor;
in vec2 TexCoords;

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;

struct Light {
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
    float Radius;
};
uniform Light light;


void main() { // Positions are in view-space
    vec3 FragPos = texture(gPositionDepth, TexCoords).xyz;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedo, TexCoords).rgb;
    
    vec3 wo  = normalize(-FragPos);
    vec3 wi = normalize(light.Position - FragPos);
    vec3 wh = normalize(wi + wo);  

    vec3 diffuse = max(dot(Normal, wi), 0.0) * Diffuse * light.Color;
    vec3 specular = 0.3 * pow(max(dot(Normal, wh), 0.0), 50.0) * light.Color;

    float distance = length(light.Position - FragPos);
    float attenuation = 1.0 / (1.0 + light.Linear * distance + light.Quadratic * distance * distance);

	FragColor = (diffuse + specular) * attenuation;
}


#version 330 core
out vec3 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;

struct Light {
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
};
uniform Light light;

void main()
{             
    // ��G��������ȡ����
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedo, TexCoords).rgb;
    float AmbientOcclusion = texture(ssao, TexCoords).r;
    
    // Blinn-Phong (�۲�ռ���)
    vec3 ambient = vec3(0.3 * AmbientOcclusion);
    vec3 viewDir  = normalize(-FragPos); 
    // ������
    vec3 lightDir = normalize(light.Position - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * light.Color;
    // ����
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    vec3 specular = 0.7 * pow(max(dot(Normal, halfwayDir), 0.0), 8.0) * light.Color;
    // ˥��
    float distance = length(light.Position - FragPos);
    float attenuation = 1.0 / (1.0 + light.Linear * distance + light.Quadratic * distance * distance);
    FragColor = (ambient + diffuse + specular) * attenuation;
}
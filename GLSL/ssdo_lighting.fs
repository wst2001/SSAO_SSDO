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


void main() {
    // 从G-buffer中提取数据
    vec3 FragPos = texture(gPositionDepth, TexCoords).xyz;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedo, TexCoords).rgb;
    // 向量
    vec3 viewDir  = normalize(-FragPos);
    vec3 lightDir = normalize(light.Position - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    // 环境光
    vec3 ambient = vec3(0.3);
    // 漫反射
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * light.Color;
    // 镜面反射
    vec3 specular = 0.3 * pow(max(dot(Normal, halfwayDir), 0.0), 8.0) * light.Color;
    // 衰减
    float distance = length(light.Position - FragPos);
    float attenuation = 1.0 / (1.0 + light.Linear * distance + light.Quadratic * distance * distance);

	FragColor = (ambient + diffuse + specular) * attenuation;
}


#version 330 core
layout (location = 0) out vec4 gPositionDepth;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

const float NEAR = 0.1;
const float FAR = 50.0;
float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));
} 

void main()
{    
    // �洢��һ��G���������е�Ƭ��λ������
    gPositionDepth.xyz = FragPos;
    gPositionDepth.w = LinearizeDepth(gl_FragCoord.z);
    // ͬ���洢��ÿ����Ƭ�η��ߵ�G������
    gNormal = normalize(Normal);
    // �������ÿ����Ƭ����ɫ
    gAlbedo.rgb = vec3(0.95);
    // �洢����ǿ�ȵ�gAlbedoSpec��alpha����
    gAlbedo.a = 0;
}
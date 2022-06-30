#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

void main()
{    
    // �洢��һ��G���������е�Ƭ��λ������
    gPosition = FragPos;
    // ͬ���洢��ÿ����Ƭ�η��ߵ�G������
    gNormal = normalize(Normal);
    // �������ÿ����Ƭ����ɫ
    gAlbedo.rgb = vec3(0.95);
    // �洢����ǿ�ȵ�gAlbedoSpec��alpha����
    gAlbedo.a = 0;
}
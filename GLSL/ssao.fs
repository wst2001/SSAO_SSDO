#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];

// ����
uniform int kernelSize;
uniform float radius;
uniform float bias;

uniform float width;
uniform float height;



uniform mat4 projection;

void main()
{
    // ��Ļ��ƽ����������������Ļ�ֱ��ʳ���������С��ֵ������
    vec2 noiseScale = vec2(width/4.0, height/4.0); 
    // ����
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    // ����TBN����
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // ����--�����߿ռ�任���۲�ռ�
    float occlusion = 0.0;
    vec3 directLight = vec3(0.0, 0.0, 0.0);
    for(int i = 0; i < kernelSize; ++i)
    {
        // ��ȡ����λ��
        vec3 samplePos = TBN * samples[i]; // ����->�۲�ռ�
        samplePos = fragPos + samplePos * radius; 
        
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset; // �۲�->�ü��ռ�
        offset.xyz /= offset.w; // ͸�ӻ���
        offset.xyz = offset.xyz * 0.5 + 0.5; // [0.0 - 1.0]
        
        // ��ȡ�������ֵ
        float sampleDepth = texture(gPosition, offset.xy).z; 
        float depth2 = texture(gPosition, offset.xy).a; // has it been drawn to ?
        
        // ���뷶Χ����
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;         
        
        
    }
    occlusion = 1.0 - (occlusion / kernelSize);
    FragColor = occlusion;  
}
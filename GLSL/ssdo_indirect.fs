#version 450 core
out vec3 FragColor;
in vec2 TexCoords;

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D texNoise;
uniform sampler2D texLighting;


uniform vec3 samples[64];
int kernelSize = 64;
float radius = 1.0;

uniform mat4 projection;
float occlusion = 0.0;
void main() {
    vec2 noiseScale = textureSize(gNormal,0) / textureSize(texNoise,0);
    // 从buffer中提取数据
    vec3 fragPos = texture(gPositionDepth, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    // TBN: 从切线空间到观察空间
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // 对每个采样点计算SSDO间接光照
    vec3 indirectLight = vec3(0.0, 0.0, 0.0);

    for(int i = 0; i < kernelSize; ++i) {
        // 采样点位置
        vec3 samplePos = TBN * samples[i]; // 从切线空间到观察空间
        samplePos = fragPos + samplePos * radius;

        // 将采样点投影到屏幕空间
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset; // 从观察空间到裁剪空间
        offset.xyz /= offset.w; // 透视除法
        offset.xyz = offset.xyz * 0.5 + 0.5; // 变换到0.0 - 1.0

        // 获取采样点深度
	    float currentDepth = samplePos.z;
        float newDepth = texture(gPositionDepth, offset.xy).z;
		vec3 sampleNormal = normalize(texture(gNormal, offset.xy).xyz);
		vec3 sampleColor = texture(texLighting, offset.xy).rgb;
        samplePos = texture(gPositionDepth, offset.xy).xyz;

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - samplePos.z));

        // 范围测试 & 累加
        if (newDepth >= currentDepth + 0.025 && rangeCheck >= 1.0) {
            occlusion += 1.0;
            float cos_th_si = max(dot(sampleNormal, normalize(fragPos - samplePos)), 0.0);
            float cos_th_ri = max(dot(normal, -normalize(fragPos - samplePos)), 0.0);
            float di = length(fragPos - samplePos);
            indirectLight += (cos_th_si * cos_th_ri) * sampleColor * (1 - di/radius);
        }
    }

	FragColor = 5 * indirectLight / occlusion;
}

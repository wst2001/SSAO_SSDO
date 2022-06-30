#version 450 core
out vec3 FragColor;
in vec2 TexCoords;

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform int kernelSize;
uniform float radius;

// tile noise texture over screen based on screen dimensions divided by noise size
uniform samplerCube skybox;

uniform mat4 projection;
uniform mat4 iview; // view to world-space

void main() {
    vec2 noiseScale = textureSize(gNormal,0) / textureSize(texNoise,0);
    // get input for SSDO algorithm
    vec3 fragPos = texture(gPositionDepth, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // iterate over the sample kernel and calculate the incoming light
    vec3 directLight = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < kernelSize; ++i) {
        // get sample position
        vec3 samplePos = TBN * samples[i]; // from tangent to view-space
        samplePos = fragPos + samplePos * radius;

        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

        // get sample depth
        float sampleDepth = texture(gPositionDepth, offset.xy).z; // get depth value of kernel sample
        float depth2 = texture(gPositionDepth, offset.xy).a; // has it been drawn to ?

        // range check & accumulate
		if (sampleDepth < samplePos.z || depth2 == 1) {
			vec4 skyboxDirection = iview * vec4(samplePos - fragPos, 0.0);
			vec3 skyboxColor = texture(skybox, skyboxDirection.xyz).xyz;
			directLight += skyboxColor * dot(normal, normalize(samplePos - fragPos));
		}
    }

    FragColor = directLight / kernelSize;
}

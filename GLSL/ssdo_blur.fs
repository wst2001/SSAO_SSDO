#version 450 core
in vec2 TexCoords;
out vec3 FragColor;

uniform sampler2D tex;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(tex, 0));
    vec3 acc = vec3(0.0, 0.0, 0.0);
    for (int x = -2; x < 2; ++x)
    {
        for (int y = -2; y < 2; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            acc += texture(tex, TexCoords + offset).rgb;
        }
    }
    FragColor = acc / 16.;
}

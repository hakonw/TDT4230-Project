#version 420 core
#extension GL_ARB_explicit_uniform_location : require

in layout(location = 0) vec2 textureCoordinates;

uniform sampler2D imageSampler;

out vec4 color;

uniform layout(location = 1) bool horizontal = true;

// Aproximation of gausian blur, precalculated values
float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);
int blurSize = 5;


void main()
{
    vec2 tex_offset = 1.0 / textureSize(imageSampler, 0); // gets size of single texel
    vec3 result = texture(imageSampler, textureCoordinates).rgb * weight[0];

    if (horizontal) {
        for (int i = 1; i < blurSize; ++i) {
            vec2 offset = vec2(tex_offset.x * i, 0.0);
            result += texture(imageSampler, textureCoordinates + offset).rgb * weight[i];
            result += texture(imageSampler, textureCoordinates - offset).rgb * weight[i];
        }
    } else {
        for (int i = 1; i < blurSize; ++i) {
            vec2 offset = vec2(0.0, tex_offset.y * i);
            result += texture(imageSampler, textureCoordinates + offset).rgb * weight[i];
            result += texture(imageSampler, textureCoordinates - offset).rgb * weight[i];
        }
    }

    // TODO recsue transparrent value, as its trhown away atm
    color = vec4(result, 1.0f);
}
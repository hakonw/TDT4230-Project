#version 420 core
#extension GL_ARB_explicit_uniform_location : require

in layout(location = 0) vec2 textureCoordinates;

uniform layout(binding = 0) sampler2D sampler;

out vec4 color;

void main()
{
    color = texture(sampler, textureCoordinates);
}
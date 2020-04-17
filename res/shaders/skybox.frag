#version 420 core
#extension GL_ARB_explicit_uniform_location : require

in layout(location = 0) vec2 textureCoordinates;

uniform layout(binding = 0) sampler2D sampler;

layout (location = 0) out vec4 color;
layout (location = 1) out vec4 brightColor;


void main()
{
    color = texture(sampler, textureCoordinates);
    brightColor = vec4(0.0f);
}
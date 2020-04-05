#version 420 core
#extension GL_ARB_explicit_uniform_location : require

in layout(location = 0) vec3 position;
in layout(location = 2) vec2 textureCoordinates_in;
uniform layout(location = 3) mat4 VP;

out layout(location = 0) vec2 textureCoordinates_out;

void main()
{
    textureCoordinates_out = textureCoordinates_in;
    vec4 pos4 = vec4(position, 1.0f);
    gl_Position = VP * pos4;
}

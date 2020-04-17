#version 420 core
#extension GL_ARB_explicit_uniform_location : require

in layout(location = 0) vec2 textureCoordinates;

uniform sampler2D scene;
uniform sampler2D blur;

out vec4 color;

void main()
{
    vec4 sceneColor = texture(scene, textureCoordinates);
    vec3 sceneColorRGB = sceneColor.rgb;
    float alpha = sceneColor.a;
    vec3 bloomColor = texture(blur, textureCoordinates).rgb;

    vec3 result = sceneColorRGB + bloomColor;

    color = vec4(result, 1.0f); // TODO use alpha
}
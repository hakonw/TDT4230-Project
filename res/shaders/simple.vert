#version 420 core
#extension GL_ARB_explicit_uniform_location : require

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;
in layout(location = 3) vec3 tangents_in;
in layout(location = 4) vec3 biTangens_in;

uniform layout(location = 3) mat4 MVP;
uniform layout(location = 4) mat4 M;
uniform layout(location = 5) mat3 normalMatrix;
uniform layout(location = 6) int is2dGeo;

out layout(location = 0) vec3 normal_out;
out layout(location = 1) vec2 textureCoordinates_out;
out layout(location = 3) vec3 fragPos_out;
out layout(location = 4) mat3 tbn_out;

void main()
{
    textureCoordinates_out = textureCoordinates_in;
    vec4 pos4 = vec4(position, 1.0f);
    gl_Position = MVP * pos4;

    // These calculations are only needed when doing 3d geo
    if (is2dGeo != 1) {
        gl_Position = MVP * pos4;

        vec3 T = normalize(mat3(M) * tangents_in);
        vec3 B = normalize(mat3(M) * biTangens_in);
        vec3 N = normalize(mat3(M) * normal_in);
        tbn_out = mat3(T,B,N);

        vec4 fragPos4 = M * pos4;
        fragPos_out = vec3(fragPos4)/fragPos4.w;

        normal_out = normalize(normalMatrix  * normal_in);
    }
}

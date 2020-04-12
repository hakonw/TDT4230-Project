#version 420 core
#extension GL_ARB_explicit_uniform_location : require

in layout(location = 0) vec3 normalUniform;
in layout(location = 1) vec2 textureCoordinates;
in layout(location = 3) vec3 fragPos;
in layout(location = 4) mat3 tbn;

// Uniforms
uniform layout(location = 10) vec3 cameraPos;
uniform layout(location = 11) vec3 ballPos; // 2c
float ballRadius = 3.0f; // 2c

uniform layout(location = 7) int useTexture;
uniform layout(binding = 1) sampler2D samplerTexture;
uniform layout(location = 8) int useNormalMap;
uniform layout(binding = 2) sampler2D samplerNormal;
uniform layout(location = 9) int useRoughnessMap;
uniform layout(binding = 3) sampler2D samplerRoughness;

uniform layout(location = 12) int ignoreLight = 0;

out vec4 color;


float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }

float ambientStrength = 0.15f;

// Task 2a
float l_constant = 0.5; // l_a,  i hate the bright light close to the source, so this is tuned WAAY down
float l_linear = 0.0001; // l_b
float l_quadratic = 0.0001; // l_c

//float noiseStrength = 0.01f;

// Task 3a
struct PointLight {
    vec3 position;
    vec3 ambientColor;
    vec3 diffuseColor;
    vec3 specularColor;
};

struct Material {
    vec3 baseColor;
    float shininess;
};

uniform Material material;

#define NUM_POINT_LIGHTS 1
uniform PointLight pointLights[NUM_POINT_LIGHTS];

// 1d
vec3 calcPointLight(PointLight pointLight, vec3 norm, vec3 fragPos, vec3 viewDir, vec3 lightDir, float lightRatio) {
    vec3 result = vec3(0.0f);

    // 2a
    float distance = length(pointLight.position - fragPos);
    float attenuation = 1.0f / (l_constant + l_linear * distance + l_quadratic * (distance*distance));

    // Ambient light
    vec3 ambient = pointLight.ambientColor*ambientStrength;
    result += ambient*attenuation; // No light ratio multiplied, as its ambient, not direct

    // Diffusion light
    float diffuseIntensity = max(dot(lightDir, norm), 0.0f);
    result += pointLight.diffuseColor*diffuseIntensity*attenuation*lightRatio;

    // Specular light 1h
    float spec = 32; // Default shinyness factor
    if (useRoughnessMap == 1) {
        vec4 roughnessSample = texture(samplerRoughness, textureCoordinates);
        float roughnessSampleValue = roughnessSample.x;
        spec = (5/(roughnessSampleValue*roughnessSampleValue));
    }
    vec3 reflectDir = reflect(-lightDir, norm);
    float specIntensity = pow(max(dot(reflectDir, viewDir), 0.0f), spec);
    result += pointLight.specularColor*specIntensity*attenuation*lightRatio;

    return result;
}

vec3 reject(vec3 from, vec3 onto) {
    return from - onto*dot(from, onto)/dot(onto, onto);
}

void main()
{

    vec3 normal = normalUniform;

    if (useNormalMap == 1) {
        vec3 n = texture(samplerNormal, textureCoordinates).xyz;
        n = (n*2.0f) - 1;
        n = tbn * n;
        normal = n;
    }

    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(cameraPos - fragPos);

    vec3 result = vec3(0.0f);

    // Calculate light from point lights
    if (ignoreLight == 0){
        for (int i=0; i<NUM_POINT_LIGHTS; i++) {
            vec3 lightVec = pointLights[i].position - fragPos;
            vec3 lightDir = normalize(lightVec);

            vec3 fragBallVec = ballPos - fragPos;
            vec3 rejection = reject(fragBallVec, lightVec);

            // Default behaviour: calculate light
            float lightRatio = 1.0f; // A value that goes between 0 and 1, which "allows" light

            // == -> lightratio = 1, reject < ballRadius -> shadow, reject > ball -> full light
            // Formula: 1-x^2, for x in range 0-1
            float rejectionLeftover = length(rejection) - ballRadius; // is negative when you are "inside" the ball

            rejectionLeftover = min(rejectionLeftover, 0.0f); // used formula is symmetric
            lightRatio = min(max(1.0f-(rejectionLeftover*rejectionLeftover), 0.0f), 1.0f); // 1 - x^2,  cap at [0, 1]
            rejectionLeftover = max(rejectionLeftover, 1.0f);
            if (length(lightVec) < length(fragBallVec)) lightRatio = 1.0f; // special case 1
            if (dot(lightVec, fragBallVec) < 0) lightRatio = 1.0f; // special case 2
            lightRatio = 1.0f; // TODO force disable of shadow (atm)

            result += calcPointLight(pointLights[i], norm, fragPos, viewDir, lightDir, lightRatio);
        }
    } else {
        result = vec3(1.0f);
    }


    result = result * material.baseColor; // light level & basecolor
    color = vec4(result, 1.0f);

    if (useTexture == 1) {
        color = texture(samplerTexture, textureCoordinates) * color;
        // To test
        //color = vec4(tbn * (texture(samplerNormal, textureCoordinates).xyz * 2.0f - 1.0f), 1.0f);
    }

    color = color + dither(textureCoordinates);

}
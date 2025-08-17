/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#include <cstring>

namespace BinaryData
{

//================== drone_fragment.glsl ==================
static const unsigned char temp_binary_data_0[] =
"#version 330 core\n"
"\n"
"// Fragment shader for rendering drones\n"
"\n"
"// Input data from vertex shader\n"
"in vec3 fragNormal;\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"in vec3 fragPosition;\n"
"\n"
"// Output color\n"
"out vec4 outColor;\n"
"\n"
"// Uniforms\n"
"uniform sampler2D textureSampler;\n"
"uniform vec3 lightPosition;\n"
"uniform vec3 cameraPosition;\n"
"uniform bool useTexture;\n"
"\n"
"void main()\n"
"{\n"
"    // Lighting calculations\n"
"    vec3 lightDir = normalize(lightPosition - fragPosition);\n"
"    vec3 viewDir = normalize(cameraPosition - fragPosition);\n"
"    vec3 normal = normalize(fragNormal);\n"
"    \n"
"    // Ambient component\n"
"    float ambientStrength = 0.3;\n"
"    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);\n"
"    \n"
"    // Diffuse component\n"
"    float diff = max(dot(normal, lightDir), 0.0);\n"
"    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);\n"
"    \n"
"    // Specular component\n"
"    float specularStrength = 0.5;\n"
"    vec3 reflectDir = reflect(-lightDir, normal);\n"
"    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
"    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);\n"
"    \n"
"    // Combine lighting with color\n"
"    vec3 lighting = ambient + diffuse + specular;\n"
"    \n"
"    // Apply texture if enabled, otherwise use drone color\n"
"    vec4 baseColor;\n"
"    if (useTexture)\n"
"        baseColor = texture(textureSampler, fragTexCoord) * fragColor;\n"
"    else\n"
"        baseColor = fragColor;\n"
"    \n"
"    // Final color\n"
"    outColor = vec4(baseColor.rgb * lighting, baseColor.a);\n"
"    \n"
"    // Add a rim lighting effect for notes that are active\n"
"    float rim = 1.0 - max(dot(viewDir, normal), 0.0);\n"
"    rim = smoothstep(0.4, 1.0, rim);\n"
"    \n"
"    if (fragColor.a > 0.9) // Trick to detect active notes (they have higher alpha)\n"
"        outColor.rgb += rim * 0.5;\n"
"}\n";

const char* drone_fragment_glsl = (const char*) temp_binary_data_0;

//================== drone_vertex.glsl ==================
static const unsigned char temp_binary_data_1[] =
"#version 330 core\n"
"\n"
"// Vertex shader for rendering drones\n"
"\n"
"// Input vertex data\n"
"layout(location = 0) in vec3 position;\n"
"layout(location = 1) in vec3 normal;\n"
"layout(location = 2) in vec2 texCoord;\n"
"layout(location = 3) in vec4 color;\n"
"\n"
"// Output data to fragment shader\n"
"out vec3 fragNormal;\n"
"out vec2 fragTexCoord;\n"
"out vec4 fragColor;\n"
"out vec3 fragPosition;\n"
"\n"
"// Uniforms\n"
"uniform mat4 projectionMatrix;\n"
"uniform mat4 viewMatrix;\n"
"uniform mat4 modelMatrix;\n"
"uniform float droneSize;\n"
"uniform vec4 droneColor;\n"
"uniform bool noteActive;\n"
"\n"
"void main()\n"
"{\n"
"    // Calculate vertex position in clip space\n"
"    vec4 worldPosition = modelMatrix * vec4(position * droneSize, 1.0);\n"
"    gl_Position = projectionMatrix * viewMatrix * worldPosition;\n"
"    \n"
"    // Calculate normal in world space\n"
"    fragNormal = normalize(mat3(modelMatrix) * normal);\n"
"    \n"
"    // Pass through texture coordinates\n"
"    fragTexCoord = texCoord;\n"
"    \n"
"    // Pass through color (modified by noteActive)\n"
"    fragColor = noteActive ? mix(droneColor, vec4(1.0, 1.0, 1.0, 1.0), 0.3) : droneColor;\n"
"    \n"
"    // Pass world position to fragment shader\n"
"    fragPosition = worldPosition.xyz;\n"
"}\n";

const char* drone_vertex_glsl = (const char*) temp_binary_data_1;

//================== trail_fragment.glsl ==================
static const unsigned char temp_binary_data_2[] =
"#version 330 core\n"
"\n"
"// Fragment shader for rendering trails\n"
"\n"
"// Input data from vertex shader\n"
"in vec4 fragColor;\n"
"\n"
"// Output color\n"
"out vec4 outColor;\n"
"\n"
"void main()\n"
"{\n"
"    // Simply use the interpolated color\n"
"    outColor = fragColor;\n"
"}\n";

const char* trail_fragment_glsl = (const char*) temp_binary_data_2;

//================== trail_vertex.glsl ==================
static const unsigned char temp_binary_data_3[] =
"#version 330 core\n"
"\n"
"// Vertex shader for rendering trails\n"
"\n"
"// Input vertex data\n"
"layout(location = 0) in vec3 position;\n"
"layout(location = 1) in vec4 color;\n"
"\n"
"// Output data to fragment shader\n"
"out vec4 fragColor;\n"
"\n"
"// Uniforms\n"
"uniform mat4 projectionMatrix;\n"
"uniform mat4 viewMatrix;\n"
"uniform mat4 modelMatrix;\n"
"uniform float fadeVal;\n"
"\n"
"void main()\n"
"{\n"
"    // Calculate vertex position in clip space\n"
"    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);\n"
"    \n"
"    // Fade color based on trail position\n"
"    fragColor = color;\n"
"    fragColor.a *= fadeVal;\n"
"}\n";

const char* trail_vertex_glsl = (const char*) temp_binary_data_3;


const char* getNamedResource (const char* resourceNameUTF8, int& numBytes);
const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;

    if (resourceNameUTF8 != nullptr)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0xd78175a6:  numBytes = 1710; return drone_fragment_glsl;
        case 0xb9490d12:  numBytes = 1132; return drone_vertex_glsl;
        case 0x56040014:  numBytes = 232; return trail_fragment_glsl;
        case 0xf6275b00:  numBytes = 574; return trail_vertex_glsl;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "drone_fragment_glsl",
    "drone_vertex_glsl",
    "trail_fragment_glsl",
    "trail_vertex_glsl"
};

const char* originalFilenames[] =
{
    "drone_fragment.glsl",
    "drone_vertex.glsl",
    "trail_fragment.glsl",
    "trail_vertex.glsl"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
        if (strcmp (namedResourceList[i], resourceNameUTF8) == 0)
            return originalFilenames[i];

    return nullptr;
}

}

#version 330 core

// Vertex shader for rendering drones

// Input vertex data
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 color;

// Output data to fragment shader
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragPosition;

// Uniforms
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform float droneSize;
uniform vec4 droneColor;
uniform bool noteActive;

void main()
{
    // Calculate vertex position in clip space
    vec4 worldPosition = modelMatrix * vec4(position * droneSize, 1.0);
    gl_Position = projectionMatrix * viewMatrix * worldPosition;
    
    // Calculate normal in world space
    fragNormal = normalize(mat3(modelMatrix) * normal);
    
    // Pass through texture coordinates
    fragTexCoord = texCoord;
    
    // Pass through color (modified by noteActive)
    fragColor = noteActive ? mix(droneColor, vec4(1.0, 1.0, 1.0, 1.0), 0.3) : droneColor;
    
    // Pass world position to fragment shader
    fragPosition = worldPosition.xyz;
}

#version 330 core

// Vertex shader for rendering trails

// Input vertex data
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

// Output data to fragment shader
out vec4 fragColor;

// Uniforms
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform float fadeVal;

void main()
{
    // Calculate vertex position in clip space
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
    
    // Fade color based on trail position
    fragColor = color;
    fragColor.a *= fadeVal;
}

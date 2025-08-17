#version 330 core

// Fragment shader for rendering trails

// Input data from vertex shader
in vec4 fragColor;

// Output color
out vec4 outColor;

void main()
{
    // Simply use the interpolated color
    outColor = fragColor;
}

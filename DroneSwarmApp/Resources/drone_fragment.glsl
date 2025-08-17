#version 330 core

// Fragment shader for rendering drones

// Input data from vertex shader
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragPosition;

// Output color
out vec4 outColor;

// Uniforms
uniform sampler2D textureSampler;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;
uniform bool useTexture;

void main()
{
    // Lighting calculations
    vec3 lightDir = normalize(lightPosition - fragPosition);
    vec3 viewDir = normalize(cameraPosition - fragPosition);
    vec3 normal = normalize(fragNormal);
    
    // Ambient component
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
    
    // Diffuse component
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    
    // Specular component
    float specularStrength = 0.5;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    
    // Combine lighting with color
    vec3 lighting = ambient + diffuse + specular;
    
    // Apply texture if enabled, otherwise use drone color
    vec4 baseColor;
    if (useTexture)
        baseColor = texture(textureSampler, fragTexCoord) * fragColor;
    else
        baseColor = fragColor;
    
    // Final color
    outColor = vec4(baseColor.rgb * lighting, baseColor.a);
    
    // Add a rim lighting effect for notes that are active
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    rim = smoothstep(0.4, 1.0, rim);
    
    if (fragColor.a > 0.9) // Trick to detect active notes (they have higher alpha)
        outColor.rgb += rim * 0.5;
}

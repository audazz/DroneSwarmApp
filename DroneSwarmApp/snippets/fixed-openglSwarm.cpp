// Modified MainComponent methods for your DroneSwarmApp
// Based on your working paste.txt example

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// === Add to MainComponent.h in the private section ===
// juce::Point<int> lastMousePosition;

// === Replace implementation of OpenGL methods in DroneSwarmApp.cpp ===

void MainComponent::newOpenGLContextCreated()
{
    juce::Logger::writeToLog("OpenGL context created");
    
    // Set up OpenGL for 3D rendering
    juce::gl::glEnable(GL_DEPTH_TEST);
    juce::gl::glDepthFunc(GL_LESS);
    
    // Enable smooth shading
    juce::gl::glShadeModel(GL_SMOOTH);
    
    // Optional: Enable alpha blending for transparency
    juce::gl::glEnable(GL_BLEND);
    juce::gl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Log OpenGL information
    const char* version = (const char*)juce::gl::glGetString(GL_VERSION);
    if (version != nullptr)
        juce::Logger::writeToLog("OpenGL version: " + juce::String(version));
}

void MainComponent::renderOpenGL()
{
    // Clear the screen with a black background
    juce::gl::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    juce::gl::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set up the viewport
    juce::gl::glViewport(0, 0, getWidth(), getHeight());
    
    // Set up the projection matrix
    juce::gl::glMatrixMode(GL_PROJECTION);
    juce::gl::glLoadIdentity();
    
    // Create perspective projection
    float fovY = 45.0f;
    float aspect = (float)getWidth() / (float)getHeight();
    float zNear = 0.1f;
    float zFar = 100.0f;
    
    float fH = std::tan(fovY * (M_PI / 360.0f)) * zNear;
    float fW = fH * aspect;
    
    juce::gl::glFrustum(-fW, fW, -fH, fH, zNear, zFar);
    
    // Set up the modelview matrix
    juce::gl::glMatrixMode(GL_MODELVIEW);
    juce::gl::glLoadIdentity();
    
    // Position the camera
    juce::gl::glTranslatef(0.0f, 0.0f, -40.0f * (1.0f / zoomLevel));
    
    // Apply the rotations
    juce::gl::glRotatef(0.0f, 1.0f, 0.0f, 0.0f);  // No X rotation
    juce::gl::glRotatef(rotationAngle * 57.2958f, 0.0f, 1.0f, 0.0f);  // Y rotation (convert from radians to degrees)
    
    // Draw each drone
    for (auto& drone : drones)
    {
        // Save current transformation
        juce::gl::glPushMatrix();
        
        // Position the drone
        juce::gl::glTranslatef(drone->position.x, drone->position.y, drone->position.z);
        
        // Scale the drone size
        float size = drone->size / 100.0f;  // Normalize size
        juce::gl::glScalef(size, size, size);
        
        // Set the drone color
        juce::gl::glColor4f(drone->colour.getFloatRed(), 
                          drone->colour.getFloatGreen(), 
                          drone->colour.getFloatBlue(),
                          drone->noteActive ? 1.0f : 0.8f);  // More opaque when playing a note
        
        // Draw the drone as an octahedron (8-sided shape)
        juce::gl::glBegin(GL_TRIANGLES);
        
        // Top half
        juce::gl::glVertex3f( 0.0f,  1.0f,  0.0f);  // Top point
        juce::gl::glVertex3f(-1.0f,  0.0f,  0.0f);
        juce::gl::glVertex3f( 0.0f,  0.0f,  1.0f);
        
        juce::gl::glVertex3f( 0.0f,  1.0f,  0.0f);  // Top point
        juce::gl::glVertex3f( 0.0f,  0.0f,  1.0f);
        juce::gl::glVertex3f( 1.0f,  0.0f,  0.0f);
        
        juce::gl::glVertex3f( 0.0f,  1.0f,  0.0f);  // Top point
        juce::gl::glVertex3f( 1.0f,  0.0f,  0.0f);
        juce::gl::glVertex3f( 0.0f,  0.0f, -1.0f);
        
        juce::gl::glVertex3f( 0.0f,  1.0f,  0.0f);  // Top point
        juce::gl::glVertex3f( 0.0f,  0.0f, -1.0f);
        juce::gl::glVertex3f(-1.0f,  0.0f,  0.0f);
        
        // Bottom half
        juce::gl::glVertex3f( 0.0f, -1.0f,  0.0f);  // Bottom point
        juce::gl::glVertex3f( 0.0f,  0.0f,  1.0f);
        juce::gl::glVertex3f(-1.0f,  0.0f,  0.0f);
        
        juce::gl::glVertex3f( 0.0f, -1.0f,  0.0f);  // Bottom point
        juce::gl::glVertex3f( 1.0f,  0.0f,  0.0f);
        juce::gl::glVertex3f( 0.0f,  0.0f,  1.0f);
        
        juce::gl::glVertex3f( 0.0f, -1.0f,  0.0f);  // Bottom point
        juce::gl::glVertex3f( 0.0f,  0.0f, -1.0f);
        juce::gl::glVertex3f( 1.0f,  0.0f,  0.0f);
        
        juce::gl::glVertex3f( 0.0f, -1.0f,  0.0f);  // Bottom point
        juce::gl::glVertex3f(-1.0f,  0.0f,  0.0f);
        juce::gl::glVertex3f( 0.0f,  0.0f, -1.0f);
        
        juce::gl::glEnd();
        
        // If note is active, draw a glowing ring
        if (drone->noteActive)
        {
            // Draw a ring when the drone is active (producing a note)
            juce::gl::glColor4f(1.0f, 1.0f, 1.0f, 0.7f);  // White, semi-transparent
            
            juce::gl::glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 16; ++i)
            {
                float angle = i * 2.0f * M_PI / 16.0f;
                float x = std::cos(angle) * 1.2f;
                float z = std::sin(angle) * 1.2f;
                juce::gl::glVertex3f(x, 0.0f, z);
            }
            juce::gl::glEnd();
        }
        
        // Restore the transformation
        juce::gl::glPopMatrix();
    }
    
    // Draw trails if enabled
    if (enableTrails)
    {
        for (auto& drone : drones)
        {
            if (drone->trail.size() < 2)
                continue;
                
            // Set trail color (semi-transparent)
            juce::gl::glColor4f(drone->colour.getFloatRed(), 
                              drone->colour.getFloatGreen(), 
                              drone->colour.getFloatBlue(),
                              0.4f);  // Semi-transparent
            
            // Draw the trail as a line strip
            juce::gl::glBegin(GL_LINE_STRIP);
            
            for (const auto& pos : drone->trail)
            {
                juce::gl::glVertex3f(pos.x, pos.y, pos.z);
            }
            
            juce::gl::glEnd();
        }
    }
    
    // Optional: Draw a grid on the ground for perspective
    juce::gl::glColor4f(0.3f, 0.3f, 0.3f, 0.5f);
    juce::gl::glBegin(GL_LINES);
    
    float gridSize = 15.0f;
    float step = 2.0f;
    
    for (float i = -gridSize; i <= gridSize; i += step)
    {
        juce::gl::glVertex3f(-gridSize, -gridSize, i);
        juce::gl::glVertex3f( gridSize, -gridSize, i);
        
        juce::gl::glVertex3f(i, -gridSize, -gridSize);
        juce::gl::glVertex3f(i, -gridSize,  gridSize);
    }
    
    juce::gl::glEnd();
}

void MainComponent::openGLContextClosing()
{
    juce::Logger::writeToLog("OpenGL context closing");
    
    // No manual cleanup needed for fixed-function pipeline
}

// === Modify the constructor to improve OpenGL setup ===

// Inside MainComponent constructor, update the OpenGL context setup:
// openGLContext.setComponentPaintingEnabled(false);  // Force OpenGL-only rendering
// openGLContext.setContinuousRepainting(true);       // Keep rendering frames

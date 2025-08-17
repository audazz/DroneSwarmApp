
#pragma once

#include <JuceHeader.h>
#include <vector>
#include <array>
#include <memory>
#include <random>
#include <functional>
#include <deque>


#if JUCE_MAC
    #include <OpenGL/OpenGL.h>
    #include <OpenGL/gl3.h>
#endif
//==============================================================================
// Forward declarations
class SwarmDrone;
class Formation;
class RhythmPattern;

//==============================================================================
/**
 * Main application class for the Generative MIDI Swarm
 */
class DroneSwarmApp : public juce::JUCEApplication
{
public:
    DroneSwarmApp();
    ~DroneSwarmApp() override;
    
    const juce::String getApplicationName() override;
    const juce::String getApplicationVersion() override;
    
    void initialise(const juce::String& commandLine) override;
    void shutdown() override;
    
    void systemRequestedQuit() override;
    void anotherInstanceStarted(const juce::String& commandLine) override;
    
private:
    std::unique_ptr<juce::DocumentWindow> mainWindow;
};


//==============================================================================
/**
 * Main component that contains the 3D visualization and controls
 */
class MainComponent : public juce::Component,
                      public juce::Timer,
                      private juce::MidiInputCallback,
                      private juce::MidiKeyboardStateListener,
                        public juce::OpenGLRenderer
{
public:
    MainComponent();
    ~MainComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Timer callback for animation updates
    void timerCallback() override;
    
    // MIDI callbacks
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    
    // Mouse and keyboard input
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;
    // Add these OpenGL callbacks
        void newOpenGLContextCreated() override;
        void renderOpenGL() override;
        void openGLContextClosing() override;
    struct Vertex {
        float x, y, z;
        float r, g, b, a;
    };

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;
  

    // In your initialization code:
    void createGeometry()
    {
        // Define your geometry data
        vertices = {
            // Example: positions and colors for a simple shape
            {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}, // red
            { 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f}, // green
            { 0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f}  // blue
        };
        
        indices = {0, 1, 2}; // Simple triangle
        numIndices = static_cast<int>(indices.size());
    }
    
private:
    // OpenGL context
       juce::OpenGLContext openGLContext;
       
       
       
       // Shader program
       std::unique_ptr<juce::OpenGLShaderProgram> shader;
    // UI Components
    
    juce::Slider chaosSlider;
    juce::Slider formationStrengthSlider;
    juce::ComboBox formationSelector;
    juce::ComboBox rhythmSelector;
    juce::ComboBox scaleSelector;
    juce::Slider rootNoteSlider;
    juce::ToggleButton trailsToggle;
    juce::TextButton pauseButton;
    
    // MIDI handling
    juce::MidiKeyboardState keyboardState;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    std::unique_ptr<juce::MidiInput> midiInput;
    
    // Swarm management
    void updateSwarm();
    void generateMidi();
    void updateFormationTargets();
    void setupMidi();
    void renderDrones(juce::Graphics& g);
    void renderTrails(juce::Graphics& g);
    void updateStatusText();
    
    // Swarm state
    std::vector<std::unique_ptr<SwarmDrone>> drones;
    std::unique_ptr<Formation> currentFormation;
    std::unique_ptr<RhythmPattern> currentRhythm;
    int numIndices =0;
    // Animation state
    int frameCount = 0;
    bool paused = false;
    float rotationAngle = 0.0f;
    float zoomLevel = 1.0f;
    
    // Settings
    float chaosLevel = 0.3f;
    float formationStrength = 0.7f;
    bool enableTrails = true;
    
    // Swarm parameters
    static constexpr int DEFAULT_NUM_DRONES = 8;
    static constexpr int UPDATE_INTERVAL_MS = 40; // 25 fps
    static constexpr int NOTE_CHECK_INTERVAL = 3; // frames
    
    // 3D visualization
    juce::Vector3D<float> cameraPosition;
    juce::Vector3D<float> cameraTarget;
    juce::Matrix3D<float> projectionMatrix;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

//==============================================================================
/**
 * Represents a single drone in the swarm
 */
class SwarmDrone
{
public:
    SwarmDrone(int id, const juce::Colour& colour);
    ~SwarmDrone();
    
    // Position and movement
    juce::Vector3D<float> position;
    juce::Vector3D<float> velocity;
    juce::Vector3D<float> targetPosition;
    
    // State
    int droneId;
    juce::Colour colour;
    float size = 100.0f;
    bool noteActive = false;
    int currentNote = 0;
    int midiChannel = 0;
    
    // Trail for visualization
    std::deque<juce::Vector3D<float>> trail;
    static constexpr size_t MAX_TRAIL_LENGTH = 20;
    
    // Update drone physics
    void update(float chaosLevel, float formationStrength);
    
    // Keep drone within bounds
    void enforceBoundaries()
    {
        // Boundary checking - bounce off edges of the space
        constexpr float boundary = 15.0f;
        constexpr float bounceFactor = -0.7f;
        
        // X boundaries
        if (position.x > boundary)
        {
            position.x = boundary - (position.x - boundary);
            velocity.x *= bounceFactor;
        }
        else if (position.x < -boundary)
        {
            position.x = -boundary - (position.x + boundary);
            velocity.x *= bounceFactor;
        }
        
        // Y boundaries
        if (position.y > boundary)
        {
            position.y = boundary - (position.y - boundary);
            velocity.y *= bounceFactor;
        }
        else if (position.y < -boundary)
        {
            position.y = -boundary - (position.y + boundary);
            velocity.y *= bounceFactor;
        }
        
        // Z boundaries
        if (position.z > boundary)
        {
            position.z = boundary - (position.z - boundary);
            velocity.z *= bounceFactor;
        }
        else if (position.z < -boundary)
        {
            position.z = -boundary - (position.z + boundary);
            velocity.z *= bounceFactor;
        }
    };
    
private:
    // Random number generation
    std::random_device rd;
    std::mt19937 rng;
};

//==============================================================================
/**
 * Base class for different formation patterns
 */
class Formation
{
public:
    Formation() = default;
    virtual ~Formation() = default;
    
    // Calculate target positions for all drones
    virtual void calculateTargets(std::vector<std::unique_ptr<SwarmDrone>>& drones, 
                                  float timeFactor) = 0;
    
    // Get name of the formation
    virtual juce::String getName() const = 0;
    
    // Factory method to create formations
    static std::unique_ptr<Formation> create(const juce::String& name);
    
    // Available formation types
    static std::vector<juce::String> getFormationTypes();
};

//==============================================================================
/**
 * Base class for different rhythm patterns
 */
class RhythmPattern
{
public:
    RhythmPattern() = default;
    virtual ~RhythmPattern() = default;
    
    // Calculate which drones should be active for a given frame
    virtual std::vector<bool> calculateActiveNotes(int numDrones, int frameCount) = 0;
    
    // Get name of the rhythm pattern
    virtual juce::String getName() const = 0;
    
    // Factory method to create rhythm patterns
    static std::unique_ptr<RhythmPattern> create(const juce::String& name);
    
    // Available rhythm types
    static std::vector<juce::String> getRhythmTypes();
};

//==============================================================================
/**
 * Handles musical scales and note mapping
 */
class MusicScales
{
public:
    MusicScales();
    ~MusicScales() = default;
    
    // Get notes for a particular scale
    std::vector<int> getScaleNotes(const juce::String& scaleName, int rootNote = 60) const;
    
    // Available scale types
    static std::vector<juce::String> getScaleTypes();
    
private:
    // Scale definitions (intervals)
    std::map<juce::String, std::vector<int>> scales;
};

//==============================================================================
/**
 * OpenGL renderer for 3D visualization
 */
class DroneSwarmRenderer
{
public:
    DroneSwarmRenderer();
    ~DroneSwarmRenderer();
    
    void setup(juce::OpenGLContext& context);
    void render(juce::OpenGLContext& context, const std::vector<std::unique_ptr<SwarmDrone>>& drones,
                float rotationAngle, float zoomLevel);
    
private:
    void createShaders();
    void createDroneMesh();
    void createTrailMesh();
    
    // OpenGL shader programs
    std::unique_ptr<juce::OpenGLShaderProgram> droneShader;
    std::unique_ptr<juce::OpenGLShaderProgram> trailShader;
    
    // Mesh data
    juce::OpenGLTexture texture;
    
    // JUCE doesn't have a VertexBuffer class, use OpenGL buffers directly
    struct BufferObjects {
        GLuint vertexBuffer = 0;
        GLuint indexBuffer = 0;
        GLuint vertexArrayObject = 0;
       
    };
   
    
    BufferObjects droneMesh;
    BufferObjects trailMesh;
};

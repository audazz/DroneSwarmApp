#include "DroneSwarmApp.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#endif

#define MY_VIEWPORT(x, y, width, height) \
    glViewport(x, y, width, height)

#define MY_DRAW_ELEMENTS(mode, count, type, indices) \
    glDrawElements(mode, count, type, indices)

#define MY_GEN_BUFFERS(n, buffers) \
    glGenBuffers(n, buffers)

#define MY_BIND_BUFFER(target, buffer) \
    glBindBuffer(target, buffer)

#define MY_BUFFER_DATA(target, size, data, usage) \
    glBufferData(target, size, data, usage)

#define MY_DELETE_BUFFERS(n, buffers) \
    glDeleteBuffers(n, buffers)

// Common OpenGL constants in case they're missing
#ifndef GL_ARRAY_BUFFER
    #define GL_ARRAY_BUFFER 0x8892
#endif

#ifndef GL_ELEMENT_ARRAY_BUFFER
    #define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif

#ifndef GL_STATIC_DRAW
    #define GL_STATIC_DRAW 0x88E4
#endif

#ifndef GL_TRIANGLES
    #define GL_TRIANGLES 0x0004
#endif

#ifndef GL_UNSIGNED_INT
    #define GL_UNSIGNED_INT 0x1405
#endif


//==============================================================================
// DroneSwarmApp implementation

DroneSwarmApp::DroneSwarmApp() = default;

DroneSwarmApp::~DroneSwarmApp() = default;

const juce::String DroneSwarmApp::getApplicationName()
{
    return ProjectInfo::projectName;
}

const juce::String DroneSwarmApp::getApplicationVersion()
{
    return ProjectInfo::versionString;
}

void DroneSwarmApp::initialise(const juce::String& commandLine)
{
    // Create main window
    mainWindow.reset(new juce::DocumentWindow(getApplicationName(),
                                              juce::Colours::darkgrey,
                                              juce::DocumentWindow::allButtons));
    
    mainWindow->setUsingNativeTitleBar(true);
    mainWindow->setContentOwned(new MainComponent(), true);
    mainWindow->centreWithSize(900, 700);
    mainWindow->setVisible(true);
}

void DroneSwarmApp::shutdown()
{
    mainWindow = nullptr;
}

void DroneSwarmApp::systemRequestedQuit()
{
    quit();
}

void DroneSwarmApp::anotherInstanceStarted(const juce::String& commandLine)
{
    // Not handling multiple instances
}

//==============================================================================
// MainComponent implementation

MainComponent::MainComponent()
{
    // Set up OpenGL rendering
    openGLContext.setRenderer(this);
    openGLContext.setComponentPaintingEnabled(true);
    openGLContext.setContinuousRepainting(true);
    openGLContext.attachTo(*this);
    
    // Make component visible and set size
    setSize(900, 700);
    setWantsKeyboardFocus(true);
    
    // Initialize camera view
    cameraPosition = juce::Vector3D<float>(0.0f, 0.0f, 40.0f);
    cameraTarget = juce::Vector3D<float>(0.0f, 0.0f, 0.0f);
    
    // Initialize UI components
    addAndMakeVisible(formationSelector);
    formationSelector.addItemList(juce::StringArray(Formation::getFormationTypes().data(),
                                                   Formation::getFormationTypes().size()), 1);
    formationSelector.setSelectedItemIndex(1); // Circle by default
    formationSelector.onChange = [this]() {
        currentFormation = Formation::create(formationSelector.getText());
    };
    
    addAndMakeVisible(rhythmSelector);
    rhythmSelector.addItemList(juce::StringArray(RhythmPattern::getRhythmTypes().data(),
                                                RhythmPattern::getRhythmTypes().size()), 1);
    rhythmSelector.setSelectedItemIndex(0); // Continuous by default
    rhythmSelector.onChange = [this]() {
        currentRhythm = RhythmPattern::create(rhythmSelector.getText());
    };
    
    addAndMakeVisible(scaleSelector);
    scaleSelector.addItemList(juce::StringArray(MusicScales::getScaleTypes().data(),
                                               MusicScales::getScaleTypes().size()), 1);
    scaleSelector.setSelectedItemIndex(1); // Major by default
    
    addAndMakeVisible(chaosSlider);
    chaosSlider.setRange(0.0, 1.0, 0.01);
    chaosSlider.setValue(chaosLevel, juce::dontSendNotification);
    chaosSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    chaosSlider.onValueChange = [this]() { chaosLevel = static_cast<float>(chaosSlider.getValue()); };
    
    addAndMakeVisible(formationStrengthSlider);
    formationStrengthSlider.setRange(0.0, 1.0, 0.01);
    formationStrengthSlider.setValue(formationStrength, juce::dontSendNotification);
    formationStrengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    formationStrengthSlider.onValueChange = [this]() {
        formationStrength = static_cast<float>(formationStrengthSlider.getValue());
    };
    
    addAndMakeVisible(rootNoteSlider);
    rootNoteSlider.setRange(36, 84, 1);
    rootNoteSlider.setValue(60, juce::dontSendNotification); // Middle C by default
    rootNoteSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    
    addAndMakeVisible(trailsToggle);
    trailsToggle.setButtonText("Enable Trails");
    trailsToggle.setToggleState(enableTrails, juce::dontSendNotification);
    trailsToggle.onClick = [this]() { enableTrails = trailsToggle.getToggleState(); };
    
    addAndMakeVisible(pauseButton);
    pauseButton.setButtonText("Pause");
    pauseButton.onClick = [this]() {
        paused = !paused;
        pauseButton.setButtonText(paused ? "Resume" : "Pause");
    };
    
    // Set up MIDI
    setupMidi();
    
    // Create drones
    for (int i = 0; i < DEFAULT_NUM_DRONES; ++i)
    {
        auto colour = juce::Colour::fromHSV(static_cast<float>(i) / DEFAULT_NUM_DRONES, 1.0f, 1.0f, 1.0f);
        drones.push_back(std::make_unique<SwarmDrone>(i, colour));
    }
    
    // Initialize formation and rhythm patterns
    currentFormation = Formation::create(formationSelector.getText());
    currentRhythm = RhythmPattern::create(rhythmSelector.getText());
    
    // Start timer for animation updates
    startTimerHz(1000 / UPDATE_INTERVAL_MS);
}

MainComponent::~MainComponent()
{
    // Stop timer
    stopTimer();
    
    // Clean up OpenGL
    openGLContext.detach();
    
    // Send all notes off
    if (midiOutput != nullptr)
    {
        for (int channel = 0; channel < 16; ++channel)
        {
            midiOutput->sendMessageNow(juce::MidiMessage::allNotesOff(channel + 1));
        }
    }
}

void MainComponent::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(juce::Colours::black);
    
    // Draw status information
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    
    juce::String statusText;
    statusText << "Formation: " << currentFormation->getName() << "   "
               << "Rhythm: " << currentRhythm->getName() << "   "
               << "Frame: " << frameCount << "   "
               << (paused ? "PAUSED" : "PLAYING");
    
    g.drawText(statusText, getLocalBounds().removeFromTop(20), juce::Justification::centred, true);
    
    // 3D rendering is handled by OpenGL
    renderDrones(g);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    
    // Set up control panel at the bottom
    auto controlsArea = area.removeFromBottom(80).reduced(10);
    
    auto row1 = controlsArea.removeFromTop(25);
    auto row2 = controlsArea.removeFromTop(25);
    
    formationSelector.setBounds(row1.removeFromLeft(150));
    row1.removeFromLeft(10);
    rhythmSelector.setBounds(row1.removeFromLeft(150));
    row1.removeFromLeft(10);
    scaleSelector.setBounds(row1.removeFromLeft(150));
    row1.removeFromLeft(10);
    rootNoteSlider.setBounds(row1.removeFromLeft(150));
    
    formationStrengthSlider.setBounds(row2.removeFromLeft(200));
    row2.removeFromLeft(10);
    chaosSlider.setBounds(row2.removeFromLeft(200));
    row2.removeFromLeft(10);
    trailsToggle.setBounds(row2.removeFromLeft(120));
    row2.removeFromLeft(10);
    pauseButton.setBounds(row2.removeFromLeft(80));
}

void MainComponent::timerCallback()
{
    if (!paused)
    {
        // Update animation
        updateSwarm();
        generateMidi();
        frameCount++;
        
        // Rotate view slightly
        rotationAngle += 0.005f;
        if (rotationAngle > juce::MathConstants<float>::twoPi)
            rotationAngle -= juce::MathConstants<float>::twoPi;
    }
    
    // Trigger a repaint
    repaint();
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    // Pass the message to the keyboard state so that we can see which keys are pressed
    keyboardState.processNextMidiEvent(message);
    
    // For future MIDI input handling if needed
}

void MainComponent::handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    // Handle incoming note-on events
    // Could be used to manually trigger drone notes or control parameters
}

void MainComponent::handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    // Handle incoming note-off events
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // Space bar toggles pause
    if (key.isKeyCode(juce::KeyPress::spaceKey))
    {
        paused = !paused;
        pauseButton.setButtonText(paused ? "Resume" : "Pause");
        return true;
    }
    
    // Number keys 1-7 select formations
    if (key.isKeyCode('1'))
    {
        formationSelector.setSelectedItemIndex(0); // Free
        currentFormation = Formation::create("Free");
        return true;
    }
    else if (key.isKeyCode('2'))
    {
        formationSelector.setSelectedItemIndex(1); // Circle
        currentFormation = Formation::create("Circle");
        return true;
    }
    else if (key.isKeyCode('3'))
    {
        formationSelector.setSelectedItemIndex(2); // Spiral
        currentFormation = Formation::create("Spiral");
        return true;
    }
    else if (key.isKeyCode('4'))
    {
        formationSelector.setSelectedItemIndex(3); // Grid
        currentFormation = Formation::create("Grid");
        return true;
    }
    else if (key.isKeyCode('5'))
    {
        formationSelector.setSelectedItemIndex(4); // Wave
        currentFormation = Formation::create("Wave");
        return true;
    }
    else if (key.isKeyCode('6'))
    {
        formationSelector.setSelectedItemIndex(5); // Flock
        currentFormation = Formation::create("Flock");
        return true;
    }
    else if (key.isKeyCode('7'))
    {
        formationSelector.setSelectedItemIndex(6); // Custom
        currentFormation = Formation::create("Custom");
        return true;
    }
    
    // Letter keys q-y select rhythm patterns
    if (key.isKeyCode('q'))
    {
        rhythmSelector.setSelectedItemIndex(0); // Continuous
        currentRhythm = RhythmPattern::create("Continuous");
        return true;
    }
    else if (key.isKeyCode('w'))
    {
        rhythmSelector.setSelectedItemIndex(1); // Alternating
        currentRhythm = RhythmPattern::create("Alternating");
        return true;
    }
    else if (key.isKeyCode('e'))
    {
        rhythmSelector.setSelectedItemIndex(2); // Sequential
        currentRhythm = RhythmPattern::create("Sequential");
        return true;
    }
    else if (key.isKeyCode('r'))
    {
        rhythmSelector.setSelectedItemIndex(3); // Wave
        currentRhythm = RhythmPattern::create("Wave");
        return true;
    }
    else if (key.isKeyCode('t'))
    {
        rhythmSelector.setSelectedItemIndex(4); // Random
        currentRhythm = RhythmPattern::create("Random");
        return true;
    }
    else if (key.isKeyCode('y'))
    {
        rhythmSelector.setSelectedItemIndex(5); // Polyrhythm
        currentRhythm = RhythmPattern::create("Polyrhythm");
        return true;
    }
    
    // Handle zoom with +/- keys
    if (key.isKeyCode(juce::KeyPress::upKey))
    {
        zoomLevel *= 1.1f;
        return true;
    }
    else if (key.isKeyCode(juce::KeyPress::downKey))
    {
        zoomLevel *= 0.9f;
        return true;
    }
    
    return false;
}

void MainComponent::mouseDown(const juce::MouseEvent& e)
{
    // Could be used for orbit or camera control
}

void MainComponent::mouseDrag(const juce::MouseEvent& e)
{
    // Could be used for orbit or camera control
}

void MainComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    // Zoom in/out with mouse wheel
    zoomLevel += wheel.deltaY;
    zoomLevel = juce::jlimit(0.1f, 5.0f, zoomLevel);
    repaint();
}

void MainComponent::updateSwarm()
{
    // Update formation targets
    updateFormationTargets();
    
    // Update all drones
    for (auto& drone : drones)
    {
        drone->update(chaosLevel, formationStrength);
    }
}

void MainComponent::generateMidi()
{
    if (midiOutput == nullptr)
        return;
    
    // Only process MIDI at intervals to reduce CPU load
    if (frameCount % NOTE_CHECK_INTERVAL != 0)
        return;
    
    // Get the active rhythm pattern
    auto activePattern = currentRhythm->calculateActiveNotes(static_cast<int>(drones.size()), frameCount);
    
    // Get current musical scale notes
    MusicScales musicScales;
    int rootNote = static_cast<int>(rootNoteSlider.getValue());
    auto scaleNotes = musicScales.getScaleNotes(scaleSelector.getText(), rootNote);
    
    // Process each drone
    for (size_t i = 0; i < drones.size(); ++i)
    {
        auto& drone = drones[i];
        float speed = drone->velocity.length();
        
        // Only trigger notes if in the active pattern and moving fast enough
        if (activePattern[i] && speed > 0.3f)
        {
            // Map position to note
            int noteIdx = static_cast<int>((drone->position.x + 15.0f) / 30.0f * scaleNotes.size());
            noteIdx = juce::jlimit(0, static_cast<int>(scaleNotes.size()) - 1, noteIdx);
            int note = scaleNotes[noteIdx];
            
            // Map Y position to velocity
            int velocity = static_cast<int>(juce::jlimit(30, 100,
                static_cast<int>((drone->position.y + 15.0f) / 30.0f * 70.0f + 30.0f)));
            
            // Map Z position to control parameters
            int ccValue = static_cast<int>(juce::jlimit(0, 127,
                static_cast<int>((drone->position.z + 15.0f) / 30.0f * 127.0f)));
            
            // MIDI channel for this drone
            int channel = drone->midiChannel;
            
            // Only send new note if different from last
            if (note != drone->currentNote)
            {
                // Send note off for previous note first
                if (drone->noteActive && drone->currentNote > 0)
                {
                    midiOutput->sendMessageNow(juce::MidiMessage::noteOff(channel + 1, drone->currentNote, 0.0f));
                    drone->noteActive = false;
                }
                
                // Send new note
                midiOutput->sendMessageNow(juce::MidiMessage::noteOn(channel + 1, note, static_cast<float>(velocity) / 127.0f));
                drone->noteActive = true;
                drone->currentNote = note;
                
                // Occasional controller messages
                if (juce::Random::getSystemRandom().nextFloat() < 0.3f)
                {
                    midiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(channel + 1, 1, ccValue));
                }
                
                // Visual feedback - increase size when note triggers
                drone->size = 150.0f;
            }
            else
            {
                drone->size = 100.0f;
            }
        }
        else
        {
            // Turn off note when drone stops or rhythm pattern doesn't include it
            if (drone->noteActive && drone->currentNote > 0)
            {
                midiOutput->sendMessageNow(juce::MidiMessage::noteOff(drone->midiChannel + 1, drone->currentNote, 0.0f));
                drone->noteActive = false;
                drone->currentNote = 0;
            }
            drone->size = 100.0f;
        }
    }
}

void MainComponent::updateFormationTargets()
{
    // Update target positions based on current formation
    if (currentFormation != nullptr)
    {
        float timeFactor = frameCount * 0.01f;
        currentFormation->calculateTargets(drones, timeFactor);
    }
}

void MainComponent::setupMidi()
{
    // Set up MIDI output
    auto midiOutputs = juce::MidiOutput::getAvailableDevices();
    
    if (midiOutputs.size() > 0)
    {
        // Use the first available MIDI output device
        midiOutput = juce::MidiOutput::openDevice(midiOutputs[0].identifier);
        
        if (midiOutput)
        {
            juce::Logger::writeToLog("Connected to MIDI output: " + midiOutputs[0].name);
        }
    }
    else
    {
        // No MIDI output devices available
        juce::Logger::writeToLog("No MIDI output devices available");
    }
    
    // Optional: Set up MIDI input for future use
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    
    if (midiInputs.size() > 0)
    {
        // Use the first available MIDI input device
        midiInput = juce::MidiInput::openDevice(midiInputs[0].identifier, this);
        
        if (midiInput)
        {
            midiInput->start();
            juce::Logger::writeToLog("Connected to MIDI input: " + midiInputs[0].name);
        }
    }
}

void MainComponent::renderDrones(juce::Graphics& g)
{
    // In a real implementation, we would use OpenGL to render the 3D scene
    // For simplicity, here we use a basic 2D approximation
    
    // Center of the view
    int centerX = getWidth() / 2;
    int centerY = getHeight() / 2;
    
    // Render perspective
    float fov = 500.0f * zoomLevel;
    
    // First render trails if enabled
    if (enableTrails)
        renderTrails(g);
    
    // Render drones as circles
    for (auto& drone : drones)
    {
        // Apply rotation
        float rotatedX = drone->position.x * std::cos(rotationAngle) - drone->position.z * std::sin(rotationAngle);
        float rotatedZ = drone->position.x * std::sin(rotationAngle) + drone->position.z * std::cos(rotationAngle);
        
        // Perspective projection
        float depth = 30.0f + rotatedZ;
        if (depth <= 0.0f) depth = 0.1f;
        
        float screenX = centerX + (rotatedX * fov) / depth;
        float screenY = centerY + (drone->position.y * fov) / depth;
        
        // Size based on depth
        float size = (drone->size / depth) * zoomLevel;
        
        // Draw the drone
        g.setColour(drone->colour);
        g.fillEllipse(screenX - size/2, screenY - size/2, size, size);
        
        // Draw outline if note is active
        if (drone->noteActive)
        {
            g.setColour(juce::Colours::white);
            g.drawEllipse(screenX - size/2, screenY - size/2, size, size, 2.0f);
        }
    }
}

void MainComponent::renderTrails(juce::Graphics& g)
{
    // Center of the view
    int centerX = getWidth() / 2;
    int centerY = getHeight() / 2;
    
    // Render perspective
    float fov = 500.0f * zoomLevel;
    
    // Render each drone's trail
    for (auto& drone : drones)
    {
        if (drone->trail.size() < 2)
            continue;
        
        // Set trail color (semi-transparent version of drone color)
        g.setColour(drone->colour.withAlpha(0.3f));
        
        // Create path for the trail
        juce::Path trailPath;
        bool firstPoint = true;
        
        for (const auto& pos : drone->trail)
        {
            // Apply rotation
            float rotatedX = pos.x * std::cos(rotationAngle) - pos.z * std::sin(rotationAngle);
            float rotatedZ = pos.x * std::sin(rotationAngle) + pos.z * std::cos(rotationAngle);
            
            // Perspective projection
            float depth = 30.0f + rotatedZ;
            if (depth <= 0.0f) depth = 0.1f;
            
            float screenX = centerX + (rotatedX * fov) / depth;
            float screenY = centerY + (pos.y * fov) / depth;
            
            if (firstPoint)
            {
                trailPath.startNewSubPath(screenX, screenY);
                firstPoint = false;
            }
            else
            {
                trailPath.lineTo(screenX, screenY);
            }
        }
        
        // Draw the trail
        g.strokePath(trailPath, juce::PathStrokeType(1.0f));
    }
}

//=====START=== modification 2025-05-15 >

void MainComponent::newOpenGLContextCreated() 
{
    // Create geometry first
    createGeometry();
    
    // Now create buffers
    openGLContext.extensions.glGenBuffers(1, &vertexBuffer);
    openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER,
                                         vertices.size() * sizeof(Vertex),
                                         vertices.data(),
                                         GL_STATIC_DRAW);
    
    openGLContext.extensions.glGenBuffers(1, &indexBuffer);
    openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    openGLContext.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                         indices.size() * sizeof(GLuint),
                                         indices.data(),
                                         GL_STATIC_DRAW);
}
void MainComponent::renderOpenGL()
{
    // Clear background
    juce::OpenGLHelpers::clear(juce::Colours::black);
    
    // Direct OpenGL calls juce::gl::GL_ARRAY_BUFFER
    juce::gl::glViewport(0, 0, getWidth(), getHeight());
    juce::gl::glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    juce::gl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    juce::gl::glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, nullptr);
    juce::gl::glBindBuffer(GL_ARRAY_BUFFER, 0);
    juce::gl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void MainComponent::openGLContextClosing()
{
    // Clean up buffers - use extensions
    openGLContext.extensions.glDeleteBuffers(1, &vertexBuffer);
    openGLContext.extensions.glDeleteBuffers(1, &indexBuffer);
    vertexBuffer = 0;
    indexBuffer = 0;
}

START_JUCE_APPLICATION( DroneSwarmApp);

//=====START=== modification 2025-05-17 >
//==============================================================================
// SwarmDrone Implementation
//==============================================================================

SwarmDrone::SwarmDrone(int id, const juce::Colour& colour)
    : droneId(id), colour(colour), rng(rd())
{
    // Initialize with random position
    std::uniform_real_distribution<float> posDist(-10.0f, 10.0f);
    position = juce::Vector3D<float>(posDist(rng), posDist(rng), posDist(rng));
    
    // Initialize with zero velocity
    velocity = juce::Vector3D<float>(0.0f, 0.0f, 0.0f);
    
    // Set initial target to current position
    targetPosition = position;
    
    // Assign MIDI channel (distribute across 1-16)
    midiChannel = id % 16;
}

SwarmDrone::~SwarmDrone() = default;

void SwarmDrone::update(float chaosLevel, float formationStrength)
{
    // Add current position to trail
    trail.push_front(position);
    if (trail.size() > MAX_TRAIL_LENGTH)
        trail.pop_back();
    
    // Calculate vector to target
    juce::Vector3D<float> toTarget = targetPosition - position;
    float distanceToTarget = toTarget.length();
    
    // Normalize the direction if not zero
    if (distanceToTarget > 0.001f)
        toTarget = toTarget / distanceToTarget;
    
    // Apply force towards target based on formation strength
    velocity += toTarget * (0.1f * formationStrength);
    
    // Add random movement (chaos)
    std::normal_distribution<float> noiseDist(0.0f, chaosLevel * 0.05f);
    velocity.x += noiseDist(rng);
    velocity.y += noiseDist(rng);
    velocity.z += noiseDist(rng);
    
    // Apply damping to prevent excessive speeds
    velocity *= 0.95f;
    
    // Limit maximum velocity
    float maxSpeed = 0.5f;
    float currentSpeed = velocity.length();
    if (currentSpeed > maxSpeed)
        velocity = velocity * (maxSpeed / currentSpeed);
    
    // Update position
    position += velocity;
    
    // Enforce boundaries
    enforceBoundaries();
}

//==============================================================================
// Formation Implementation
//==============================================================================

// Define concrete formation classes

// Free formation - drones move randomly
class FreeFormation : public Formation
{
public:
    void calculateTargets(std::vector<std::unique_ptr<SwarmDrone>>& drones, float timeFactor) override
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-15.0f, 15.0f);
        
        for (auto& drone : drones)
        {
            // Occasionally change target to a new random position
            if (std::rand() % 100 < 5)
            {
                drone->targetPosition = juce::Vector3D<float>(
                    dist(gen), dist(gen), dist(gen)
                );
            }
        }
    }
    
    juce::String getName() const override { return "Free"; }
};

// Circle formation
class CircleFormation : public Formation
{
public:
    void calculateTargets(std::vector<std::unique_ptr<SwarmDrone>>& drones, float timeFactor) override
    {
        int numDrones = static_cast<int>(drones.size());
        float radius = 10.0f;
        
        for (int i = 0; i < numDrones; ++i)
        {
            float angle = (static_cast<float>(i) / numDrones) * juce::MathConstants<float>::twoPi;
            
            // Calculate position on the circle
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;
            
            // Set the target with a slight Y offset based on index
            drones[i]->targetPosition = juce::Vector3D<float>(
                x,
                (i % 2 == 0) ? 2.0f : -2.0f, // Alternate up/down
                z
            );
        }
    }
    
    juce::String getName() const override { return "Circle"; }
};

// Spiral formation
class SpiralFormation : public Formation
{
public:
    void calculateTargets(std::vector<std::unique_ptr<SwarmDrone>>& drones, float timeFactor) override
    {
        int numDrones = static_cast<int>(drones.size());
        float baseRadius = 5.0f;
        float height = 12.0f;
        
        for (int i = 0; i < numDrones; ++i)
        {
            float t = static_cast<float>(i) / numDrones;
            float angle = t * 4.0f * juce::MathConstants<float>::twoPi + timeFactor;
            
            // Calculate position on the spiral
            float radius = baseRadius + t * 5.0f;
            float x = std::cos(angle) * radius;
            float y = height * (0.5f - t);
            float z = std::sin(angle) * radius;
            
            drones[i]->targetPosition = juce::Vector3D<float>(x, y, z);
        }
    }
    
    juce::String getName() const override { return "Spiral"; }
};

// Grid formation
class GridFormation : public Formation
{
public:
    void calculateTargets(std::vector<std::unique_ptr<SwarmDrone>>& drones, float timeFactor) override
    {
        int numDrones = static_cast<int>(drones.size());
        
        // Calculate grid dimensions
        int gridSize = static_cast<int>(std::ceil(std::sqrt(numDrones)));
        float spacing = 5.0f;
        float offset = spacing * (gridSize - 1) * 0.5f;
        
        for (int i = 0; i < numDrones; ++i)
        {
            int row = i / gridSize;
            int col = i % gridSize;
            
            // Calculate grid position
            float x = spacing * col - offset;
            float z = spacing * row - offset;
            
            // Add some sinusoidal vertical movement
            float y = 2.0f * std::sin(timeFactor + static_cast<float>(i) * 0.2f);
            
            drones[i]->targetPosition = juce::Vector3D<float>(x, y, z);
        }
    }
    
    juce::String getName() const override { return "Grid"; }
};

// Wave formation
class WaveFormation : public Formation
{
public:
    void calculateTargets(std::vector<std::unique_ptr<SwarmDrone>>& drones, float timeFactor) override
    {
        int numDrones = static_cast<int>(drones.size());
        float width = 15.0f;
        float depth = 10.0f;
        
        for (int i = 0; i < numDrones; ++i)
        {
            // Distribute drones evenly across the width
            float t = static_cast<float>(i) / std::max(1, numDrones - 1);
            float x = width * (t - 0.5f);
            
            // Create a wave pattern
            float phase = timeFactor * 0.5f + t * juce::MathConstants<float>::twoPi;
            float y = 3.0f * std::sin(phase);
            float z = depth * (0.5f - t) * std::cos(phase * 0.5f);
            
            drones[i]->targetPosition = juce::Vector3D<float>(x, y, z);
        }
    }
    
    juce::String getName() const override { return "Wave"; }
};

// Flock formation with boids-like behavior
class FlockFormation : public Formation
{
    std::vector<juce::Vector3D<float>> velocities;
    
public:
    FlockFormation() = default;
    
    void calculateTargets(std::vector<std::unique_ptr<SwarmDrone>>& drones, float timeFactor) override
    {
        int numDrones = static_cast<int>(drones.size());
        
        // Initialize velocities if needed
        if (velocities.size() != numDrones)
        {
            velocities.clear();
            velocities.reserve(numDrones);
            
            for (int i = 0; i < numDrones; ++i)
            {
                velocities.push_back(juce::Vector3D<float>(
                    std::sin(i * 0.1f) * 0.1f,
                    std::cos(i * 0.2f) * 0.1f,
                    std::sin(i * 0.3f + 0.5f) * 0.1f
                ));
            }
        }
        
        // Update flock behavior
        for (int i = 0; i < numDrones; ++i)
        {
            // Calculate cohesion, separation, and alignment
            juce::Vector3D<float> cohesion(0, 0, 0);
            juce::Vector3D<float> separation(0, 0, 0);
            juce::Vector3D<float> alignment(0, 0, 0);
            
            int cohesionCount = 0;
            int separationCount = 0;
            int alignmentCount = 0;
            
            for (int j = 0; j < numDrones; ++j)
            {
                if (i == j) continue;
                
                float distance = (drones[i]->position - drones[j]->position).length();
                
                // Cohesion: steer towards center of neighbors
                if (distance < 10.0f)
                {
                    cohesion += drones[j]->position;
                    cohesionCount++;
                }
                
                // Separation: avoid crowding neighbors
                if (distance < 5.0f)
                {
                    separation += (drones[i]->position - drones[j]->position) / std::max(0.1f, distance);
                    separationCount++;
                }
                
                // Alignment: match velocity of neighbors
                if (distance < 7.0f)
                {
                    alignment += velocities[j];
                    alignmentCount++;
                }
            }
            
            // Combine all forces
            juce::Vector3D<float> force(0, 0, 0);
            
            // Apply cohesion
            if (cohesionCount > 0)
            {
                cohesion = cohesion / static_cast<float>(cohesionCount) - drones[i]->position;
                force += cohesion * 0.01f;
            }
            
            // Apply separation
            if (separationCount > 0)
            {
                separation = separation / static_cast<float>(separationCount);
                force += separation * 0.04f;
            }
            
            // Apply alignment
            if (alignmentCount > 0)
            {
                alignment = alignment / static_cast<float>(alignmentCount);
                force += alignment * 0.02f;
            }
            
            // Add some wandering behavior
            float t = timeFactor + i * 0.1f;
            juce::Vector3D<float> wander(
                std::sin(t) * 0.02f,
                std::cos(t * 1.3f) * 0.01f,
                std::sin(t * 0.7f) * 0.02f
            );
            force += wander;
            
            // Add boundary avoidance
            const float boundary = 14.0f;
            const float avoidStrength = 0.1f;
            
            if (drones[i]->position.x > boundary)
                force.x -= avoidStrength;
            else if (drones[i]->position.x < -boundary)
                force.x += avoidStrength;
                
            if (drones[i]->position.y > boundary)
                force.y -= avoidStrength;
            else if (drones[i]->position.y < -boundary)
                force.y += avoidStrength;
                
            if (drones[i]->position.z > boundary)
                force.z -= avoidStrength;
            else if (drones[i]->position.z < -boundary)
                force.z += avoidStrength;
            
            // Update velocity
            velocities[i] += force;
            
            // Limit velocity
            float maxSpeed = 0.3f;
            float speed = velocities[i].length();
            if (speed > maxSpeed)
                velocities[i] = velocities[i] * (maxSpeed / speed);
            
            // Set target just ahead of current position
            drones[i]->targetPosition = drones[i]->position + velocities[i] * 5.0f;
        }
    }
    
    juce::String getName() const override { return "Flock"; }
};

// Custom formation (can be modified for special patterns)
class CustomFormation : public Formation
{
public:
    void calculateTargets(std::vector<std::unique_ptr<SwarmDrone>>& drones, float timeFactor) override
    {
        int numDrones = static_cast<int>(drones.size());
        
        // Create a double helix pattern
        for (int i = 0; i < numDrones; ++i)
        {
            float t = static_cast<float>(i) / numDrones;
            float height = 15.0f * (0.5f - t);
            
            // First half for one helix, second half for the other
            bool firstHelix = (i < numDrones / 2);
            float offset = firstHelix ? 0.0f : juce::MathConstants<float>::pi;
            int index = firstHelix ? i : (i - numDrones / 2);
            float normalizedIndex = firstHelix ?
                static_cast<float>(index) / (numDrones / 2) :
                static_cast<float>(index) / (numDrones - numDrones / 2);
            
            // Calculate position on helix
            float angle = normalizedIndex * 4.0f * juce::MathConstants<float>::twoPi + timeFactor + offset;
            float radius = 8.0f;
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;
            
            drones[i]->targetPosition = juce::Vector3D<float>(x, height, z);
        }
    }
    
    juce::String getName() const override { return "Custom"; }
};

// Factory method to create formations
std::unique_ptr<Formation> Formation::create(const juce::String& name)
{
    if (name == "Free")
        return std::make_unique<FreeFormation>();
    else if (name == "Circle")
        return std::make_unique<CircleFormation>();
    else if (name == "Spiral")
        return std::make_unique<SpiralFormation>();
    else if (name == "Grid")
        return std::make_unique<GridFormation>();
    else if (name == "Wave")
        return std::make_unique<WaveFormation>();
    else if (name == "Flock")
        return std::make_unique<FlockFormation>();
    else if (name == "Custom")
        return std::make_unique<CustomFormation>();
    
    // Default to circle if not found
    return std::make_unique<CircleFormation>();
}

// Available formation types
std::vector<juce::String> Formation::getFormationTypes()
{
    return {
        "Free",
        "Circle",
        "Spiral",
        "Grid",
        "Wave",
        "Flock",
        "Custom"
    };
}

//==============================================================================
// RhythmPattern Implementation
//==============================================================================

// Define concrete rhythm pattern classes

// Continuous rhythm - all drones active
class ContinuousRhythm : public RhythmPattern
{
public:
    std::vector<bool> calculateActiveNotes(int numDrones, int frameCount) override
    {
        // All drones are active
        return std::vector<bool>(numDrones, true);
    }
    
    juce::String getName() const override { return "Continuous"; }
};

// Alternating rhythm - every other drone
class AlternatingRhythm : public RhythmPattern
{
public:
    std::vector<bool> calculateActiveNotes(int numDrones, int frameCount) override
    {
        std::vector<bool> active(numDrones, false);
        
        // Switch pattern every 30 frames
        bool evenActive = (frameCount / 30) % 2 == 0;
        
        for (int i = 0; i < numDrones; ++i)
        {
            active[i] = (i % 2 == 0) ? evenActive : !evenActive;
        }
        
        return active;
    }
    
    juce::String getName() const override { return "Alternating"; }
};

// Sequential rhythm - one drone at a time
class SequentialRhythm : public RhythmPattern
{
public:
    std::vector<bool> calculateActiveNotes(int numDrones, int frameCount) override
    {
        std::vector<bool> active(numDrones, false);
        
        // Activate one drone at a time, cycling through
        int activeIndex = (frameCount / 10) % numDrones;
        active[activeIndex] = true;
        
        return active;
    }
    
    juce::String getName() const override { return "Sequential"; }
};

// Wave rhythm - activation moves like a wave
class WaveRhythm : public RhythmPattern
{
public:
    std::vector<bool> calculateActiveNotes(int numDrones, int frameCount) override
    {
        std::vector<bool> active(numDrones, false);
        
        // Create a wave of activation
        for (int i = 0; i < numDrones; ++i)
        {
            float phase = static_cast<float>(frameCount) * 0.05f +
                          static_cast<float>(i) / numDrones * juce::MathConstants<float>::twoPi;
            
            active[i] = std::sin(phase) > 0.0f;
        }
        
        return active;
    }
    
    juce::String getName() const override { return "Wave"; }
};

// Random rhythm - random activation
class RandomRhythm : public RhythmPattern
{
public:
    std::vector<bool> calculateActiveNotes(int numDrones, int frameCount) override
    {
        std::vector<bool> active(numDrones, false);
        
        // Update pattern every 20 frames
        static int lastUpdateFrame = -1;
        static std::vector<bool> pattern;
        
        if (frameCount / 20 != lastUpdateFrame / 20 || pattern.size() != active.size())
        {
            lastUpdateFrame = frameCount;
            pattern.resize(numDrones);
            
            // Generate new random pattern
            for (int i = 0; i < numDrones; ++i)
            {
                pattern[i] = (std::rand() % 100) < 30; // 30% chance of being active
            }
        }
        
        return pattern;
    }
    
    juce::String getName() const override { return "Random"; }
};

// Polyrhythm - different patterns for different drones
class PolyrhythmRhythm : public RhythmPattern
{
public:
    std::vector<bool> calculateActiveNotes(int numDrones, int frameCount) override
    {
        std::vector<bool> active(numDrones, false);
        
        // Divide drones into groups with different rhythms
        for (int i = 0; i < numDrones; ++i)
        {
            int group = i % 3; // Three different groups
            
            switch (group)
            {
                case 0: // Group 1: Every 8 frames
                    active[i] = (frameCount % 8) == 0;
                    break;
                    
                case 1: // Group 2: Every 12 frames
                    active[i] = (frameCount % 12) == 0;
                    break;
                    
                case 2: // Group 3: Every 16 frames
                    active[i] = (frameCount % 16) == 0;
                    break;
            }
        }
        
        return active;
    }
    
    juce::String getName() const override { return "Polyrhythm"; }
};

// Factory method to create rhythm patterns
std::unique_ptr<RhythmPattern> RhythmPattern::create(const juce::String& name)
{
    if (name == "Continuous")
        return std::make_unique<ContinuousRhythm>();
    else if (name == "Alternating")
        return std::make_unique<AlternatingRhythm>();
    else if (name == "Sequential")
        return std::make_unique<SequentialRhythm>();
    else if (name == "Wave")
        return std::make_unique<WaveRhythm>();
    else if (name == "Random")
        return std::make_unique<RandomRhythm>();
    else if (name == "Polyrhythm")
        return std::make_unique<PolyrhythmRhythm>();
    
    // Default to continuous if not found
    return std::make_unique<ContinuousRhythm>();
}

// Available rhythm types
std::vector<juce::String> RhythmPattern::getRhythmTypes()
{
    return {
        "Continuous",
        "Alternating",
        "Sequential",
        "Wave",
        "Random",
        "Polyrhythm"
    };
}

//==============================================================================
// MusicScales Implementation
//==============================================================================

MusicScales::MusicScales()
{
    // Define common scales by their intervals
    scales["Chromatic"] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    scales["Major"] = {0, 2, 4, 5, 7, 9, 11};
    scales["Minor"] = {0, 2, 3, 5, 7, 8, 10};
    scales["Pentatonic"] = {0, 2, 4, 7, 9};
    scales["Blues"] = {0, 3, 5, 6, 7, 10};
    scales["Dorian"] = {0, 2, 3, 5, 7, 9, 10};
    scales["Phrygian"] = {0, 1, 3, 5, 7, 8, 10};
    scales["Mixolydian"] = {0, 2, 4, 5, 7, 9, 10};
    scales["Lydian"] = {0, 2, 4, 6, 7, 9, 11};
    scales["Locrian"] = {0, 1, 3, 5, 6, 8, 10};
}

std::vector<int> MusicScales::getScaleNotes(const juce::String& scaleName, int rootNote) const
{
    std::vector<int> result;
    
    // Find the scale
    auto scaleIt = scales.find(scaleName);
    if (scaleIt == scales.end())
    {
        // Default to chromatic if scale not found
        scaleIt = scales.find("Chromatic");
    }
    
    const auto& intervals = scaleIt->second;
    
    // Generate 3 octaves worth of notes
    for (int octave = 0; octave < 3; ++octave)
    {
        for (int interval : intervals)
        {
            int note = rootNote + octave * 12 + interval;
            if (note >= 0 && note < 128) // Stay within MIDI note range
                result.push_back(note);
        }
    }
    
    return result;
}

std::vector<juce::String> MusicScales::getScaleTypes()
{
    return {
        "Chromatic",
        "Major",
        "Minor",
        "Pentatonic",
        "Blues",
        "Dorian",
        "Phrygian",
        "Mixolydian",
        "Lydian",
        "Locrian"
    };
}

//==============================================================================
// DroneSwarmRenderer Implementation
//==============================================================================

// This class is not needed with the fixed-function pipeline approach,
// but we'll provide a minimal implementation for compatibility

DroneSwarmRenderer::DroneSwarmRenderer() = default;
DroneSwarmRenderer::~DroneSwarmRenderer() = default;

void DroneSwarmRenderer::setup(juce::OpenGLContext& context)
{
    // No setup needed with fixed-function pipeline
}

void DroneSwarmRenderer::render(juce::OpenGLContext& context,
                               const std::vector<std::unique_ptr<SwarmDrone>>& drones,
                               float rotationAngle, float zoomLevel)
{
    // Rendering is now handled directly in MainComponent::renderOpenGL
}

void DroneSwarmRenderer::createShaders()
{
    // Not needed with fixed-function pipeline
}

void DroneSwarmRenderer::createDroneMesh()
{
    // Not needed with fixed-function pipeline
}

void DroneSwarmRenderer::createTrailMesh()
{
    // Not needed with fixed-function pipeline
}

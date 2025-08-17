// Implementation of the drone swarm classes

#include "DroneSwarmApp.h"
#include <algorithm>
#include <cmath>
#include <numeric>

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

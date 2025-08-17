# JUCE Drone Swarm - C++ Implementation Guide

This document provides a guide for building and extending the JUCE-based Drone Swarm MIDI generator.

[DroneSwarm](file:theApp.png)

## Overview

The DroneSwarmApp is a robust MIDI generative system that visualizes drone swarms in 3D space and maps their movements to MIDI notes and control messages. The system leverages JUCE's cross-platform capabilities for audio, MIDI, GUI, and OpenGL rendering.

## Features

- **3D Drone Visualization**: OpenGL-based rendering with proper lighting and perspective
- **Multiple Formation Patterns**: Circle, Spiral, Grid, Wave, Flock, Custom, and Free
- **Rhythmic Patterns**: Continuous, Alternating, Sequential, Wave, Random, and Polyrhythm
- **Musical Mapping**: Position to notes with various scales and mappings
- **Real-time Control**: Interactive UI and keyboard shortcuts for live performance
- **Trail Visualization**: Optional history trails for each drone
- **MIDI Integration**: Full MIDI output with notes and control change messages

## Project Structure

```
DroneSwarmApp/
├── DroneSwarmApp.h                 # Main header file with class definitions
├── DroneSwarmApp.cpp               # Implementation of application
├── Resources/                      # Resource files (shaders, etc.)
│   ├── drone_vertex.glsl           # Vertex shader for drones
│   ├── drone_fragment.glsl         # Fragment shader for drones
│   ├── trail_vertex.glsl           # Vertex shader for trails
│   └── trail_fragment.glsl         # Fragment shader for trails
└── JUCE/                           # JUCE library (submodule)
```

## Key Components

### 1. DroneSwarmApp

Main application class that initializes the window and UI.

### 2. MainComponent

Central component that handles:
- User interface and controls
- Animation update loop
- MIDI generation
- OpenGL rendering setup

### 3. SwarmDrone

Class representing individual drones with:
- 3D position, velocity, and target tracking
- MIDI state (note, channel, active state)
- Trail for visualization
- Physics and boundary handling

### 4. Formation Classes

Abstract base class with concrete implementations for each formation type:
- Free: Random movement without specific formation
- Circle: Drones arranged in a circular pattern
- Spiral: 3D spiral pattern with height variation
- Grid: Organized grid with dynamic Z-axis movement
- Wave: Undulating wave pattern
- Flock: Bird-like flocking with leader and followers
- Custom: Complex formation with rotating elements

### 5. Rhythm Pattern Classes

Classes that determine which drones can trigger notes at specific times:
- Continuous: All drones can trigger at any time
- Alternating: Alternates between odd and even drones
- Sequential: Only one drone active at a time
- Wave: Moving wave pattern through the drone array
- Random: Random triggering with varying density
- Polyrhythm: Different rhythmic cycles for drone groups

### 6. MusicScales

Handles musical mapping with various scales:
- Maps positions to scale-appropriate notes
- Supports chromatic, major, minor, pentatonic, blues, and modal scales

### 7. OpenGL Rendering

Visualization is handled through:
- DroneSwarmRenderer: Manages OpenGL rendering
- Shader programs: Separate shaders for drones and trails
- 3D projection and lighting calculations

## Controls

### Keyboard Shortcuts

- **Space**: Pause/Resume animation
- **1-7**: Change formation (1=Free, 2=Circle, 3=Spiral, etc.)
- **q-y**: Change rhythm pattern (q=Continuous, w=Alternating, etc.)
- **+/-**: Zoom in/out
- **Arrow Up/Down**: Zoom in/out

### UI Controls

- **Formation Selector**: Choose drone formation pattern
- **Rhythm Selector**: Choose rhythmic pattern
- **Scale Selector**: Choose musical scale
- **Root Note Slider**: Set root note for the scale
- **Formation Strength Slider**: Control how strongly drones follow formation
- **Chaos Slider**: Control amount of random movement
- **Trails Toggle**: Enable/disable movement trails
- **Pause Button**: Pause/Resume animation

## Extending the Project

### Adding New Formations

1. Create a new class derived from `Formation`
2. Implement the `calculateTargets` method
3. Add the formation to the factory method in `Formation::create`
4. Add the formation name to `Formation::getFormationTypes`

### Adding New Rhythm Patterns

1. Create a new class derived from `RhythmPattern`
2. Implement the `calculateActiveNotes` method
3. Add the pattern to the factory method in `RhythmPattern::create`
4. Add the pattern name to `RhythmPattern::getRhythmTypes`

### Adding New Scales

Add new scale definitions to the `MusicScales` constructor

### Enhancing MIDI Mapping

Modify the `generateMidi` method in `MainComponent` to create more complex mappings

## OpenGL Improvements

For better 3D rendering:

1. Complete the `DroneSwarmRenderer` class implementation
2. Create proper 3D meshes for drones
3. Implement instanced rendering for better performance
4. Add post-processing effects for visual appeal

## Performance Considerations

- Use instanced rendering for drones
- Optimize MIDI message generation
- Consider multi-threading for physics updates
- Profile and optimize the OpenGL rendering pipeline

## Future Enhancements

- External MIDI control support
- Audio visualization based on MIDI output
- Network synchronization for multi-computer performances
- Automated drone choreography based on audio analysis
- Machine learning for generative formations and rhythms

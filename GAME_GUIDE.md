# 🎮 Dodge Game - Simple Game Example

A simple but complete game demonstrating SIMPEngine's capabilities.

## Game Overview

**Objective**: Survive as long as possible without touching the red enemy squares!

### Controls

| Key | Action |
|-----|--------|
| **W** | Move Up |
| **A** | Move Left |
| **S** | Move Down |
| **D** | Move Right |
| **ESC** | Restart Game |

## How to Play

1. **You are the blue square** in the center of the screen
2. **Enemies are red squares** that bounce around randomly
3. **Avoid the enemies** - if you touch one, the game ends
4. **Your score** is the number of seconds you've survived
5. **Press ESC** to restart after game over

## Game Features Demonstrated

### ✨ Core Engine Features Used

- **Entity Component System (ECS)**
  - Multiple entities (player + 3 enemies)
  - Components: Transform, Velocity, Collision, Render, Tag

- **Input Handling**
  - Keyboard input detection (WASD)
  - ESC key for restart

- **Scene Management**
  - Single scene with dynamic entities
  - Scene reset on game over

- **Rendering**
  - Colored squares (blue player, red enemies)
  - RenderComponent for visual representation

- **Physics-like Movement**
  - Velocity-based movement
  - Screen boundary collision (enemies bounce)
  - Player boundary clamping

- **Collision Detection**
  - AABB collision checking
  - Player-enemy collision detection
  - Game over on collision

- **Game State Management**
  - Score tracking
  - Game over state
  - State reset

### 📊 Technical Implementation

**File**: `Game/main.cpp` (230 lines)

**Key Classes**:
```cpp
class GameState          // Tracks score and game state
class DodgeGame          // Main game application
  - InitializeGame()     // Setup scene and entities
  - UpdatePlayer()       // Handle input and movement
  - UpdateEnemies()      // Enemy AI and physics
  - CheckCollisions()    // Collision detection
  - UpdateScore()        // Score incrementing
  - HandleRestart()      // Reset game
```

**Custom Component Usage**:
```cpp
VelocityComponent   // Movement data
RenderComponent     // Visual representation
TransformComponent  // Position, rotation, scale
CollisionComponent  // Collision bounds
TagComponent        // Entity type identification
CameraComponent     // Camera setup
```

## Running the Game

### Build
```bash
cd build
make -j$(nproc)
```

### Run (with Editor)
```bash
./Game/GameApp
```

### Run (Game Only, no Editor)
```bash
./Game/GameApp  # Automatically disables editor
```

## Game Difficulty

**Current Setup**:
- 3 enemies
- Move speed: 300 units/sec
- Enemy speed: 100-150 units/sec random
- Arena: 1024x800 pixels

### Make it Harder
Modify in `CreateEnemies()`:
```cpp
CreateEnemies(scene, 5);  // More enemies (default: 3)

// Increase move speed
float moveSpeed = 400.0f;  // Was 300.0f

// Make enemies faster
std::uniform_real_distribution<float> velX(-200, 200);  // Was -150, 150
std::uniform_real_distribution<float> velY(-200, 200);
```

### Make it Easier
```cpp
CreateEnemies(scene, 1);  // Fewer enemies
float moveSpeed = 400.0f;  // Faster player
// Or make enemies slower
```

## Game Architecture

```
DodgeGame (Application)
    ↓
Create Scene "DodgeArena"
    ↓
Create Entities:
    ├─ Player (Blue Square)
    │  ├─ TransformComponent (position, scale)
    │  ├─ VelocityComponent (movement)
    │  ├─ CollisionComponent (hitbox)
    │  ├─ RenderComponent (appearance)
    │  └─ TagComponent (identification)
    │
    ├─ Enemy_0 (Red Square)
    ├─ Enemy_1 (Red Square)
    ├─ Enemy_2 (Red Square)
    │  └─ Same components as player
    │
    └─ MainCamera
       └─ CameraComponent (render camera)
    ↓
Main Loop:
    1. Update Player (input handling)
    2. Update Enemies (physics and bouncing)
    3. Check Collisions
    4. Update Score
    5. Handle Restart (ESC key)
    6. Render all entities
```

## Code Highlights

### Custom Collision Detection
```cpp
bool CheckAABBCollision(const SDL_FRect& rect1, const SDL_FRect& rect2) {
    return !(rect1.x + rect1.w < rect2.x || 
             rect2.x + rect2.w < rect1.x || 
             rect1.y + rect1.h < rect2.y || 
             rect2.y + rect2.h < rect1.y);
}
```

### Input Handling
```cpp
if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_W)) {
    velocity.vy = -moveSpeed;
}
```

### ECS Query Pattern
```cpp
auto allEntities = registry.view<TransformComponent, VelocityComponent, TagComponent>();
for (auto entity : allEntities) {
    auto& tag = registry.get<TagComponent>(entity);
    if (tag.Tag == "Player") {
        // Found player
    }
}
```

## Extending the Game

### Add Collectibles
```cpp
CreatePowerup(scene);  // Add a new entity type

auto& powerup = entity.AddComponent<PowerupComponent>();
powerup.type = PowerupType::Speed;  // Different types
```

### Add Levels
```cpp
CreateEnemies(scene, 5 + level);  // More enemies per level
// Increase difficulty over time
```

### Add Sound Effects
```cpp
// Would require audio system
AudioSystem::Play("collision.wav");
AudioSystem::Play("powerup.wav");
```

### Add Particles
```cpp
CreateParticles(enemyPos, 10);  // Explosion on collision
```

## Tips for Game Development

✅ **What This Game Demonstrates**:
- Simple entity creation and destruction
- Component-based architecture
- ECS pattern for organizing game logic
- Input handling
- Collision detection
- Basic game state management
- Scene management

✅ **Best Practices Shown**:
- Using tags for entity identification
- Custom game logic in application class
- Proper component usage
- Registry queries for entity iteration
- Clear separation of concerns

## Learning Outcomes

After studying this example, you'll understand:

1. How to create a complete game with SIMPEngine
2. How to structure game logic around ECS
3. How to handle input and movement
4. How to detect collisions
5. How to manage game state
6. How to update and restart scenes
7. How to use the Component system effectively

## Next Steps

- **Modify the game**: Change colors, speeds, enemy count
- **Add features**: Power-ups, scoring system, levels
- **Study the docs**: Read docs/QUICK_REFERENCE.md for API details
- **Try editor**: Run with editor to visualize entities in real-time
- **Create your own**: Use this as a template for your games

---

**Enjoy your game!** 🎮

For more examples and documentation, see: `/docs/GETTING_STARTED.md`

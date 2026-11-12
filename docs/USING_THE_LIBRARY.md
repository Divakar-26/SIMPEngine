# Using SIMPEngine as a library

This guide is for building your own game in a **separate project** that links the installed engine. If you want the editor, work inside this repository instead (see [HOW_TO.md](HOW_TO.md#start-a-new-game)); the editor is not part of the installed library.

## 1. Get the source

The third-party libraries are git submodules, so clone recursively:

```sh
git clone --recursive https://github.com/Divakar-26/SIMPEngine.git
cd SIMPEngine
```

If you already cloned without `--recursive`:

```sh
git submodule update --init --recursive
```

## 2. Build and install

```sh
cmake --preset release
cmake --build --preset release --target Engine
sudo cmake --install build/release --prefix /usr/local --component Unspecified
```

`--component Unspecified` installs only the library. Without it CMake also runs the editor install rules, which fail unless the whole project was built (`SIMPPlayer` and the other apps are missing after `--target Engine`).

Without root, install into your home folder instead and tell CMake where to look later:

```sh
cmake --install build/release --prefix ~/.local --component Unspecified
```

What gets installed (`<prefix>` is `/usr/local` or `~/.local`):

| Path | Contents |
|---|---|
| `<prefix>/lib/simpengine/` | `libEngine.a` and the libraries it needs (SDL3, SDL3_image, yaml-cpp, spdlog, ImGui, glad, AccelEngine) |
| `<prefix>/include/simpengine/` | Engine headers (`Engine/...`) and the third-party headers they use |
| `<prefix>/lib/cmake/SIMPEngine/` | `SIMPEngineConfig.cmake` for `find_package` |
| `<prefix>/share/simpengine/assets/fonts/` | Default fonts the engine loads at runtime |

Everything lives in `simpengine` subfolders, so it does not replace any SDL3 you already have.

To uninstall, delete those four folders.

## 3. Create a project

```
MyGame/
├── CMakeLists.txt
├── main.cpp
└── assets/
    └── fonts/          copy of <prefix>/share/simpengine/assets/fonts
```

```sh
mkdir -p MyGame/assets
cp -r /usr/local/share/simpengine/assets/fonts MyGame/assets/
```

**CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyGame CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SIMPEngine REQUIRED)

add_executable(MyGame main.cpp)
target_link_libraries(MyGame PRIVATE SIMPEngine::Engine)
```

`SIMPEngine::Engine` carries the include paths, compile definitions, every dependency in the right order, OpenGL, and an rpath to the installed shared libraries. You don't add anything else.

**main.cpp**

```cpp
#include <Engine/SIMPEngine.h>
#include <Engine/Core/VFS.h>

class MyGame : public SIMPEngine::Application
{
public:
    MyGame()
    {
        auto scenes = std::make_shared<SIMPEngine::SceneManager>();
        auto scene = std::make_shared<SIMPEngine::Scene>("Main");
        scenes->AddScene("Main", scene);
        scenes->SetActiveScene("Main");

        auto camera = scene->CreateEntity("Camera");
        camera.AddComponent<CameraComponent>(1.0f, glm::vec2(0.0f)).primary = true;

        auto box = scene->CreateEntity("Box");
        auto &shape = box.AddComponent<RenderComponent>();
        shape.width = 200.0f;
        shape.height = 200.0f;
        shape.color = {240, 170, 70, 255};

        PushOverlay(std::make_unique<SIMPEngine::RenderingLayer>(scenes));
    }
};

SIMPEngine::Application *SIMPEngine::CreateApplication() { return new MyGame(); }

int main()
{
    SIMPEngine::Log::Init();
    SIMPEngine::VFS::Mount("assets", "../assets");
    auto *app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
}
```

## 4. Build and run

```sh
cd MyGame
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/MyGame
```

If you installed to `~/.local`, add `-DCMAKE_PREFIX_PATH=$HOME/.local` to the first command.

`VFS::Mount("assets", "../assets")` searches upward from the executable's folder for `../assets`. With the executable in `MyGame/build/`, it finds `MyGame/assets/`. Keep that layout when you ship: `bin/MyGame` next to `assets/`.

## 5. Autocomplete in VS Code

IntelliSense needs the exact compiler flags, which CMake writes to `build/compile_commands.json` when `CMAKE_EXPORT_COMPILE_COMMANDS` is on (it is in the template above). Point VS Code at it.

**With the C/C++ extension (ms-vscode.cpptools)**, create `MyGame/.vscode/c_cpp_properties.json`:

```json
{
    "configurations": [
        {
            "name": "Linux",
            "compileCommands": "${workspaceFolder}/build/compile_commands.json",
            "cppStandard": "c++20",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
```

Or install the **CMake Tools** extension and set `"C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"` in `.vscode/settings.json`; it feeds IntelliSense straight from CMake.

**With clangd**, it reads `compile_commands.json` automatically from the project root or `build/`.

Then run *C/C++: Reset IntelliSense Database* once. If autocomplete still fails:

| Symptom | Cause |
|---|---|
| `#include <Engine/...>` is underlined | `compile_commands.json` missing or at another path; rebuild once, check the path in the config |
| Engine types unknown, C++20 features flagged | `cppStandard` is lower than `c++20`, or a `c_cpp_properties.json` configuration without `compileCommands` is overriding the default |
| Works for `main.cpp`, not for headers | Normal: headers inherit flags from the `.cpp` that includes them; open the `.cpp` first |

## 6. What you can and can't use

- Everything in [API_GUIDE.md](API_GUIDE.md) is available: scenes, components, scripts, rendering, lighting, particles, tilemaps, audio, physics, serialization.
- The editor (`EditorLayer`, panels, gizmos) is **not** installed. Build scenes in code, or author them with the editor in this repository, save them as YAML, and load them with `SceneSerializer` or `SceneManager::LoadScene`.
- Scripts are not stored in scene files. After loading a scene, bind your scripts in code.

## 7. Updating

After pulling new engine changes:

```sh
git submodule update --init --recursive
cmake --build --preset release --target Engine
sudo cmake --install build/release --prefix /usr/local --component Unspecified
```

Then rebuild your game.

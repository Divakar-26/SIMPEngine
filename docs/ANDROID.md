# Android

SIMPEngine games run on Android through the same player the desktop uses (`SIMPPlayer`), built as a shared library and loaded by SDL's `SDLActivity`. The renderer switches to OpenGL ES 3.0 (the `SIMP_GLES` build option, always on for Android), assets are packed into the APK, and on-screen touch controls feed the input actions.

## Build an APK

Needs the Android SDK, NDK r28 (`ndk;28.2.13676358`), JDK 17 and CMake 3.22 or newer. `ANDROID_HOME` should point at the SDK.

```sh
Android/build.sh path/to/MyProject debug
```

The script packs the project (`project.simp`, `assets/`, `input.yaml`) and the engine fonts into `Android/app/src/main/assets`, runs Gradle and prints the APK path (`Android/app/build/outputs/apk/debug/app-debug.apk`). Pass Gradle properties after the variant:

| Property | Meaning |
|---|---|
| `-Pabis=arm64-v8a,x86_64` | ABIs to build (default both; `arm64-v8a` for phones, `x86_64` for the emulator) |
| `-PappId=com.me.mygame` | application id (default `org.simp.player`) |
| `-PappVersionCode=3 -PappVersionName=1.2` | version shown by Android |

`Android/tools/pack_project.py <project> --label "My Game"` only packs the files (the app name is the project name unless `--label` is given).

```sh
adb install -r Android/app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n org.simp.player/.PlayerActivity
```

## What the player does on Android

- At the first start (and whenever the packed file list changes) it unpacks `simp_files.txt` and the files under it into the app's private storage, then opens that folder like a normal project. Saves and the player's rebinds live in the app's preference folder as on desktop.
- The window is fullscreen and follows `window.orientation` of `project.simp` (landscape by default, `portrait` or `auto`; `Android/tools/pack_project.py` turns it into the manifest setting). The project's `display` settings (game resolution, aspect, stretch) apply as on desktop, so `aspect: keep` gives black bars on tall or wide phones.
- Touch: the first finger that is not on a control is the mouse (UI buttons, Pickable, Lua mouse code work). A virtual stick on the left drives `MoveX` and `MoveY`, two buttons on the right drive `Jump` and `Interact`. Controls are sized in device-independent pixels (clamped between a minimum and maximum, so they stay thumb-sized on tablets and phones) and have generous grab areas. Lua: `Input.IsTouchDevice()`, `Input.SetTouchControls(false)` hides the overlay (for a menu or a game with its own touch UI), `Input.SetFloatingStick(true)` makes the stick appear wherever the thumb lands in the left part of the screen. The controls are injected into the input actions, so the character controller and any script that reads actions works unchanged.
- Multitouch: only one finger is the mouse; every other free finger is tracked separately and never steals the pointer. `Input.GetTouches()` returns a list of `{id, x, y, worldX, worldY, pressed, released}` (viewport pixels and world position, edges last one frame). `Input.GetPinch()` returns `scale, centerX, centerY, panX, panY` for the first two free fingers (`scale` is this frame's distance ratio, 1 when no pinch), which is all a pinch-zoom or two-finger pan needs.
- Desktop parity: the stick and buttons also hold virtual keys (Left/Right/Up/Down and A/D/W/S past half deflection, Space for Jump, E for Interact), so scripts that read raw keys with `Input.IsKeyDown` work on a phone. Virtual keys do not feed the input actions (those get the analog stick value directly). A pinch also scrolls the mouse wheel (zoom code written for the wheel works). After the pointer finger lifts the mouse is parked off-screen on the next frame, so hover states (UI buttons, `Button` scaling) end instead of sticking, while the release frame still reports the release position.
- The Android Back button (and a mouse's right button that Android maps to Back) is trapped and arrives as `Key.BACK` plus `Escape`, so `Pause`/`UICancel` and Escape-driven menus work; it no longer closes the app. `Game.Quit()` closes the game (in the editor it stops Play).
- The touch controls stay inside the display's safe area (notches, rounded corners), hide while a gamepad is connected, and reset when the app goes to the background or loses focus. `Window.GetSafeArea()` returns `x, y, width, height` in viewport pixels for placing your own HUD.
- Shader Canvas files may start with a desktop `#version` (330/400 core) or `#version 300 es`; the line is swapped for the right one on each platform.
- To try the touch path on a desktop run any player with `SIMP_TOUCH=1`: the mouse then produces finger events.

## Differences from desktop

- OpenGL ES 3.0: float render targets need `EXT_color_buffer_float`; without it lighting, bloom and glow fall back to 8 bit buffers (a warning is logged). Shader Canvas shaders get `#version 300 es` plus `precision highp float;` in front, so write them without their own `#version` line (the shipped examples already do).
- No SDL_image, no editor, no Lua `system` tricks that need a shell; video capture is desktop only.
- `AccelEngine` is built without `-march=native` when cross compiling.
- Audio is WAV and OGG as on desktop.

## Testing without a phone

```sh
cmake --preset gles && cmake --build --preset gles
./build/gles/bin/SIMPPlayer path/to/MyProject
```

This runs the exact GLES code path on a desktop driver that exposes ES contexts (Mesa does). The log prints `GL_VERSION: OpenGL ES 3.2 ...` when it worked.

## Known gaps

- Only the debug APK was tried (on an x86_64 emulator image); release signing and an `.aab` are not set up. Add a `signingConfig` to `Android/app/build.gradle` for a store build.
- Pausing and resuming the app, GL context loss, the on-screen keyboard and haptics are untested.
- The launcher icon is SDL's default; replace the files in `Android/app/src/main/res/mipmap-*`.

## Running on devices from the editor

When `adb` is found (`ANDROID_HOME`, `ANDROID_SDK_ROOT`, `~/Android/Sdk`, `%LOCALAPPDATA%\Android\Sdk` or `PATH`), the transport bar shows a phone button and a device picker, and the Run menu gets **Run on Android Device** (Ctrl+Shift+F5). Without the SDK nothing appears.

- The picker lists every connected device and emulator (refreshed every 2.5 s). Tick several to deploy to all of them; unauthorized devices are greyed out. If the Android emulator is installed, its AVDs can be started from the same popup.
- Run saves the scene, builds the project with `Android/build.sh` for only the ABIs of the selected devices, installs the APK with `adb install -r` and starts it. Output goes to the Console (`[Android]`). The button turns into a stop button while it runs.
- Each project gets its own package id (`org.simp.<project name>`), so projects do not overwrite each other on the device.
- The editor needs the engine's `Android/` folder (it searches upward from the editor executable) and bash, so building from the editor works on Linux and macOS. On Windows run `Android/build.sh` from a shell.

### Setting up the tools (optional)

Everything Android in the editor is optional. Editor Settings > Android shows what was found (SDK, adb, emulator, NDK, JDK, CMake and the engine's Android folder), lets you point at an SDK folder, rescan, turn the feature off, and links to the downloads. Detection looks at `ANDROID_HOME`/`ANDROID_SDK_ROOT`, Android Studio's default folders on Windows, Linux and macOS, `sdk.dir` in `Android/local.properties`, and `adb` on `PATH`. Android Studio's bundled JDK is used when no `JAVA_HOME` is set.

- Run on Device only needs adb (SDK Platform-Tools).
- Building from the editor also needs the SDK, a JDK and the engine's `Android/` folder (the NDK and CMake are fetched or found by Gradle). On Windows the editor runs the packer with Python and `gradlew.bat`.

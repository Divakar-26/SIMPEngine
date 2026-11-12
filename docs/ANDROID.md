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
- Touch: the first finger that is not on a control is the mouse (UI buttons, Pickable, Lua mouse code work). A virtual stick on the left drives `MoveX` and `MoveY`, two buttons on the right drive `Jump` and `Interact`. Lua: `Input.IsTouchDevice()` and `Input.SetTouchControls(false)` hides the overlay (for a menu or a game with its own touch UI). The controls are injected into the input actions, so the character controller and any script that reads actions works unchanged.
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

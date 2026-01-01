# LakeSnes Android Port

This is the Android port of the LakeSnes SNES emulator using SDL2.

## Features

- Full SNES emulation using SDL2
- Touch-based virtual gamepad
- Fullscreen support
- Support for .smc, .sfc, and .zip ROM files
- Save/Load state functionality
- Fast forward support
- Battery save support

## Building

### Prerequisites

- Android Studio or Android SDK
- Android NDK
- SDL2 development libraries

### Build Instructions

1. Clone the repository
2. Open the project in Android Studio
3. Build the project using Gradle:
   ```bash
   ./gradlew assembleDebug
   ```

### Debug Build Workflow

The project includes a GitHub Actions workflow for automated debug builds. The workflow:
- Builds debug APK
- Runs unit tests
- Runs connected tests on Android emulator
- Uploads build artifacts

## Installation

1. Enable "Unknown sources" in Android settings
2. Install the generated APK from `app/build/outputs/apk/debug/`

## Controls

### Virtual Gamepad
- **D-Pad**: Up, Down, Left, Right
- **Action Buttons**: A, B, X, Y
- **Shoulder Buttons**: L, R
- **Start/Select**: Menu buttons

### Touch Controls
- Tap and hold buttons to press
- Multiple buttons can be pressed simultaneously

## Project Structure

```
app/
├── src/main/
│   ├── java/com/lakesnes/emulator/
│   │   └── EmulatorActivity.java    # Main Android activity
│   ├── cpp/
│   │   ├── CMakeLists.txt           # Native build configuration
│   │   └── emulator_jni.cpp         # JNI bridge and SDL integration
│   ├── res/
│   │   ├── layout/                  # Android UI layouts
│   │   ├── values/                  # String and style resources
│   │   └── drawable/                # App icons and graphics
│   └── AndroidManifest.xml          # App configuration
└── build.gradle                     # App-level build configuration
```

## Native Code Integration

The Android port uses JNI to communicate between the Java Android activity and the native C++ SDL-based emulator. The key components:

- **emulator_jni.cpp**: Implements JNI methods and adapts SDL for Android
- **Surface handling**: Uses Android SurfaceView for rendering
- **Audio**: Uses Android's OpenSL ES audio backend
- **File access**: Uses Android's Storage Access Framework

## Troubleshooting

### Build Issues
- Ensure Android NDK is properly installed
- Check that SDL2 headers are available
- Verify CMake version compatibility

### Runtime Issues
- Check logcat for native crashes
- Ensure ROM files are in supported formats
- Verify storage permissions are granted

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly on multiple devices
5. Submit a pull request

## License

This project maintains the same license as the original LakeSnes project.
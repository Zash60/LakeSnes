#include <jni.h>
#include <android/log.h>
#include <string>
#include <thread>
#include <mutex>

#include <SDL.h>

#include "snes.h"
#include "zip.h"

#define LOG_TAG "LakeSnes"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static struct {
    // rendering
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    // audio
    SDL_AudioDeviceID audioDevice;
    int audioFrequency;
    int16_t* audioBuffer;
    // snes, timing
    Snes* snes;
    float wantedFrames;
    int wantedSamples;
    // loaded rom
    bool loaded;
    char* romName;
    char* savePath;
    char* statePath;
    // Android specific
    bool paused;
    bool fastForward;
    bool surfaceReady;
    int surfaceWidth;
    int surfaceHeight;
    std::mutex renderMutex;
    std::thread emulatorThread;
    bool running;
} glb = {};

static uint8_t* readFileFromUri(JNIEnv* env, const char* uri, int* length);
static uint8_t* readFile(const char* name, int* length);
static void loadRomFromPath(const char* path);
static void closeRom(void);
static void setPaths(const char* path);
static void setTitle(const char* path);
static bool checkExtention(const char* name, bool forZip);
static void playAudio(void);
static void renderScreen(void);
static void handleInput(int button, bool pressed);
static void emulatorLoop(void);

// JNI implementations
extern "C" {

JNIEXPORT void JNICALL
Java_com_lakesnes_emulator_EmulatorActivity_nativeSurfaceChanged(JNIEnv* env, jobject thiz, jint width, jint height) {
    LOGI("Surface changed: %dx%d", width, height);
    
    std::lock_guard<std::mutex> lock(glb.renderMutex);
    
    glb.surfaceWidth = width;
    glb.surfaceHeight = height;
    glb.surfaceReady = true;
    
    if (!glb.window) {
        // Initialize SDL if not already done
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
            LOGE("Failed to init SDL: %s", SDL_GetError());
            return;
        }
        
        // Create window for Android
        glb.window = SDL_CreateWindow("LakeSnes", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                     width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
        if(glb.window == NULL) {
            LOGE("Failed to create window: %s", SDL_GetError());
            return;
        }
        
        glb.renderer = SDL_CreateRenderer(glb.window, -1, SDL_RENDERER_ACCELERATED);
        if(glb.renderer == NULL) {
            LOGE("Failed to create renderer: %s", SDL_GetError());
            return;
        }
        
        SDL_RenderSetLogicalSize(glb.renderer, 512, 480);
        glb.texture = SDL_CreateTexture(glb.renderer, SDL_PIXELFORMAT_ARGB8888, 
                                      SDL_TEXTUREACCESS_STREAMING, 512, 480);
        if(glb.texture == NULL) {
            LOGE("Failed to create texture: %s", SDL_GetError());
            return;
        }
        
        // Set up audio for Android
        glb.audioFrequency = 48000;
        SDL_AudioSpec want, have;
        SDL_memset(&want, 0, sizeof(want));
        want.freq = glb.audioFrequency;
        want.format = AUDIO_S16;
        want.channels = 2;
        want.samples = 2048;
        want.callback = NULL;
        
        glb.audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if(glb.audioDevice == 0) {
            LOGE("Failed to open audio device: %s", SDL_GetError());
        } else {
            glb.audioBuffer = (int16_t*)malloc(glb.audioFrequency / 50 * 4);
            SDL_PauseAudioDevice(glb.audioDevice, 0);
        }
        
        // Initialize SNES
        glb.snes = snes_init();
        glb.wantedFrames = 1.0 / 60.0;
        glb.wantedSamples = glb.audioFrequency / 60;
        glb.loaded = false;
        glb.paused = true;
        glb.fastForward = false;
        glb.running = true;
        
        LOGI("Android SDL initialized successfully");
    }
}

JNIEXPORT void JNICALL
Java_com_lakesnes_emulator_EmulatorActivity_nativeSurfaceDestroyed(JNIEnv* env, jobject thiz) {
    LOGI("Surface destroyed");
    
    std::lock_guard<std::mutex> lock(glb.renderMutex);
    glb.surfaceReady = false;
}

JNIEXPORT void JNICALL
Java_com_lakesnes_emulator_EmulatorActivity_nativeTouchEvent(JNIEnv* env, jobject thiz, jint action, jfloat x, jfloat y) {
    // Handle touch events for gamepad overlay if needed
    LOGI("Touch event: action=%d, x=%.2f, y=%.2f", action, x, y);
}

JNIEXPORT void JNICALL
Java_com_lakesnes_emulator_EmulatorActivity_nativeButtonPressed(JNIEnv* env, jobject thiz, jint button, jboolean pressed) {
    handleInput(button, pressed);
}

JNIEXPORT void JNICALL
Java_com_lakesnes_emulator_EmulatorActivity_nativeLoadRom(JNIEnv* env, jobject thiz, jstring path) {
    const char* pathStr = env->GetStringUTFChars(path, 0);
    if (pathStr) {
        loadRomFromPath(pathStr);
        env->ReleaseStringUTFChars(path, pathStr);
    }
}

JNIEXPORT void JNICALL
Java_com_lakesnes_emulator_EmulatorActivity_nativePause(JNIEnv* env, jobject thiz) {
    glb.paused = !glb.paused;
    LOGI("Emulator %s", glb.paused ? "paused" : "resumed");
}

JNIEXPORT void JNICALL
Java_com_lakesnes_emulator_EmulatorActivity_nativeResume(JNIEnv* env, jobject thiz) {
    if (glb.paused) {
        glb.paused = false;
        LOGI("Emulator resumed");
    }
}

JNIEXPORT void JNICALL
Java_com_lakesnes_emulator_EmulatorActivity_nativeReset(JNIEnv* env, jobject thiz) {
    if (glb.snes && glb.loaded) {
        snes_reset(glb.snes, false);
        LOGI("Emulator reset");
    }
}

JNIEXPORT void JNICALL
Java_com_lakesnes_emulator_EmulatorActivity_nativeSetFastForward(JNIEnv* env, jobject thiz, jboolean fastForward) {
    glb.fastForward = fastForward;
    LOGI("Fast forward %s", fastForward ? "enabled" : "disabled");
}

JNIEXPORT void JNICALL
Java_com_lakesnes_emulator_EmulatorActivity_nativeSaveState(JNIEnv* env, jobject thiz) {
    if (glb.snes && glb.loaded && glb.statePath) {
        int size = snes_saveState(glb.snes, NULL);
        uint8_t* stateData = (uint8_t*)malloc(size);
        snes_saveState(glb.snes, stateData);
        FILE* f = fopen(glb.statePath, "wb");
        if(f != NULL) {
            fwrite(stateData, size, 1, f);
            fclose(f);
            LOGI("State saved");
        } else {
            LOGE("Failed to save state");
        }
        free(stateData);
    }
}

JNIEXPORT void JNICALL
Java_com_lakesnes_emulator_EmulatorActivity_nativeLoadState(JNIEnv* env, jobject thiz) {
    if (glb.snes && glb.loaded && glb.statePath) {
        int size = 0;
        uint8_t* stateData = readFile(glb.statePath, &size);
        if (stateData != NULL) {
            if(snes_loadState(glb.snes, stateData, size)) {
                LOGI("State loaded");
            } else {
                LOGE("Failed to load state");
            }
            free(stateData);
        } else {
            LOGE("Failed to read state file");
        }
    }
}

} // extern "C"

// Helper functions
static void handleInput(int button, bool pressed) {
    if (!glb.snes) return;
    
    // Map Android buttons to SNES controller buttons
    // 0: up, 1: down, 2: left, 3: right, 4: a, 5: b, 6: x, 7: y, 8: l, 9: r, 10: select, 11: start
    int snesButton = -1;
    switch(button) {
        case 0: snesButton = 4; break; // up
        case 1: snesButton = 5; break; // down
        case 2: snesButton = 6; break; // left
        case 3: snesButton = 7; break; // right
        case 4: snesButton = 8; break; // a
        case 5: snesButton = 9; break; // b
        case 6: snesButton = 10; break; // x
        case 7: snesButton = 11; break; // y
        case 8: snesButton = 2; break; // l (shoulder)
        case 9: snesButton = 3; break; // r (shoulder)
        case 10: snesButton = 0; break; // select
        case 11: snesButton = 1; break; // start
    }
    
    if (snesButton >= 0) {
        snes_setButtonState(glb.snes, 1, snesButton, pressed);
    }
}

static void playAudio() {
    if (!glb.snes || !glb.audioBuffer) return;
    
    snes_setSamples(glb.snes, glb.audioBuffer, glb.wantedSamples);
    if(SDL_GetQueuedAudioSize(glb.audioDevice) <= glb.wantedSamples * 4 * 6) {
        SDL_QueueAudio(glb.audioDevice, glb.audioBuffer, glb.wantedSamples * 4);
    }
}

static void renderScreen() {
    if (!glb.texture || !glb.snes) return;
    
    void* pixels = NULL;
    int pitch = 0;
    if(SDL_LockTexture(glb.texture, NULL, &pixels, &pitch) != 0) {
        LOGE("Failed to lock texture: %s", SDL_GetError());
        return;
    }
    snes_setPixels(glb.snes, (uint8_t*) pixels);
    SDL_UnlockTexture(glb.texture);
    
    std::lock_guard<std::mutex> lock(glb.renderMutex);
    if (glb.surfaceReady) {
        SDL_RenderClear(glb.renderer);
        SDL_RenderCopy(glb.renderer, glb.texture, NULL, NULL);
        SDL_RenderPresent(glb.renderer);
    }
}

static void emulatorLoop() {
    LOGI("Starting emulator loop");
    
    uint64_t countFreq = SDL_GetPerformanceFrequency();
    uint64_t lastCount = SDL_GetPerformanceCounter();
    float timeAdder = 0.0;
    
    while(glb.running) {
        uint64_t curCount = SDL_GetPerformanceCounter();
        uint64_t delta = curCount - lastCount;
        lastCount = curCount;
        float seconds = delta / (float) countFreq;
        timeAdder += seconds;
        
        // Handle SDL events
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            // Handle window events, quit events, etc.
            if(event.type == SDL_QUIT) {
                glb.running = false;
            }
        }
        
        // Run emulator frame
        if(glb.loaded && !glb.paused) {
            while(timeAdder >= glb.wantedFrames - 0.002) {
                timeAdder -= glb.wantedFrames;
                
                if(glb.fastForward) {
                    snes_runFrame(glb.snes);
                }
                snes_runFrame(glb.snes);
                playAudio();
                renderScreen();
            }
        }
        
        // Small sleep to prevent 100% CPU usage
        SDL_Delay(1);
    }
    
    LOGI("Emulator loop ended");
}

// File reading and ROM loading functions (simplified versions of the original)
static uint8_t* readFile(const char* name, int* length) {
    FILE* f = fopen(name, "rb");
    if(f == NULL) return NULL;
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    rewind(f);
    uint8_t* buffer = (uint8_t*)malloc(size);
    if(fread(buffer, size, 1, f) != 1) {
        fclose(f);
        return NULL;
    }
    fclose(f);
    *length = size;
    return buffer;
}

static void loadRomFromPath(const char* path) {
    LOGI("Loading ROM: %s", path);
    
    int length = 0;
    uint8_t* file = readFile(path, &length);
    
    if(file == NULL) {
        LOGE("Failed to read file '%s'", path);
        return;
    }
    
    closeRom();
    
    if(snes_loadRom(glb.snes, file, length)) {
        setPaths(path);
        glb.wantedFrames = 1.0 / (glb.snes->palTiming ? 50.0 : 60.0);
        glb.wantedSamples = glb.audioFrequency / (glb.snes->palTiming ? 50 : 60);
        glb.loaded = true;
        glb.paused = false;
        
        LOGI("ROM loaded successfully");
        
        // Start emulator thread if not running
        if (!glb.emulatorThread.joinable()) {
            glb.emulatorThread = std::thread(emulatorLoop);
        }
    } else {
        LOGE("Failed to load ROM");
    }
    
    free(file);
}

static void closeRom() {
    if(!glb.loaded) return;
    
    int size = snes_saveBattery(glb.snes, NULL);
    if(size > 0) {
        uint8_t* saveData = (uint8_t*)malloc(size);
        snes_saveBattery(glb.snes, saveData);
        FILE* f = fopen(glb.savePath, "wb");
        if(f != NULL) {
            fwrite(saveData, size, 1, f);
            fclose(f);
            LOGI("Saved battery data");
        } else {
            LOGE("Failed to save battery data");
        }
        free(saveData);
    }
}

static void setPaths(const char* path) {
    // Simplified path handling for Android
    // In a real implementation, you'd use Android's storage APIs
    if(glb.romName) free(glb.romName);
    glb.romName = strdup(path);
    
    // Set up save and state paths in app's private storage
    glb.savePath = strdup("/data/data/com.lakesnes.emulator/saves.srm");
    glb.statePath = strdup("/data/data/com.lakesnes.emulator/state.lss");
}
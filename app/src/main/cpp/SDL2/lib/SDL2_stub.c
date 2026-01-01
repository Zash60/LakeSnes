#include "SDL.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Minimal SDL2 implementation for Android emulator
// This provides stub functions that allow the emulator to compile and run

#ifdef __cplusplus
extern "C" {
#endif

// Global state for SDL
static bool sdl_initialized = false;
static uint64_t performance_frequency = 1000000000LL; // nanoseconds per second

int SDL_Init(uint32_t flags) {
    sdl_initialized = true;
    // Initialize performance frequency directly to avoid circular dependency
    performance_frequency = 1000000000LL; // nanoseconds per second
    return 0; // Success
}

void SDL_Quit(void) {
    sdl_initialized = false;
}

const char* SDL_GetError(void) {
    return "No error"; // Stub implementation
}

SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, uint32_t flags) {
    return (SDL_Window*)malloc(sizeof(int)); // Stub - return non-null pointer
}

void SDL_DestroyWindow(SDL_Window* window) {
    if (window) free(window);
}

SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, uint32_t flags) {
    return (SDL_Renderer*)malloc(sizeof(int)); // Stub
}

void SDL_DestroyRenderer(SDL_Renderer* renderer) {
    if (renderer) free(renderer);
}

int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h) {
    return 0; // Success
}

void SDL_RenderClear(SDL_Renderer* renderer) {
    // Stub - no operation
}

int SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const void* srcrect, const void* dstrect) {
    return 0; // Success
}

void SDL_RenderPresent(SDL_Renderer* renderer) {
    // Stub - no operation for rendering
}

SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, uint32_t format, int access, int w, int h) {
    return (SDL_Texture*)malloc(sizeof(int)); // Stub
}

void SDL_DestroyTexture(SDL_Texture* texture) {
    if (texture) free(texture);
}

int SDL_LockTexture(SDL_Texture* texture, const void* rect, void** pixels, int* pitch) {
    // Allocate a simple buffer for pixel data
    static char buffer[512 * 480 * 4]; // 512x480 ARGB8888
    *pixels = buffer;
    *pitch = 512 * 4; // bytes per line
    return 0; // Success
}

void SDL_UnlockTexture(SDL_Texture* texture) {
    // Stub - nothing to do
}

// Audio functions
SDL_AudioDeviceID SDL_OpenAudioDevice(const char* device, int iscapture, const void* desired, void* obtained, int allowed_changes) {
    return 1; // Return a non-zero device ID
}

void SDL_CloseAudioDevice(SDL_AudioDeviceID dev) {
    // Stub - nothing to close
}

void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on) {
    // Stub - audio is always "playing" in this minimal implementation
}

uint32_t SDL_GetQueuedAudioSize(SDL_AudioDeviceID dev) {
    return 0; // No queued audio in stub implementation
}

int SDL_QueueAudio(SDL_AudioDeviceID dev, const void* data, uint32_t len) {
    return 0; // Success
}

// Timing functions
uint64_t SDL_GetPerformanceFrequency(void) {
    return performance_frequency;
}

uint64_t SDL_GetPerformanceCounter(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

void SDL_Delay(uint32_t ms) {
    usleep(ms * 1000); // Convert ms to microseconds
}

// Event functions
int SDL_PollEvent(SDL_Event* event) {
    return 0; // No events in stub implementation
}

// Memory functions
void* SDL_malloc(size_t size) {
    return malloc(size);
}

void SDL_free(void* memory) {
    free(memory);
}

// Utility functions
void SDL_memset(void* dst, int c, size_t len) {
    memset(dst, c, len);
}

#ifdef __cplusplus
}
#endif
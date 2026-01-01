#ifndef SDL_H
#define SDL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Basic SDL2 types
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef uint32_t SDL_AudioDeviceID;

// SDL2 constants
#define SDL_INIT_VIDEO 0x00000001
#define SDL_INIT_AUDIO 0x00000010
#define SDL_WINDOW_SHOWN 0x00000004
#define SDL_WINDOW_ALLOW_HIGHDPI 0x00004000
#define SDL_RENDERER_ACCELERATED 0x00000002
#define SDL_PIXELFORMAT_ARGB8888 0x16362004
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_QUIT 256

// SDL2 audio format
#define AUDIO_S16 0x8010

// SDL2 functions
int SDL_Init(uint32_t flags);
void SDL_Quit(void);
const char* SDL_GetError(void);

SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, uint32_t flags);
void SDL_DestroyWindow(SDL_Window* window);

SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, uint32_t flags);
void SDL_DestroyRenderer(SDL_Renderer* renderer);
int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h);
void SDL_RenderClear(SDL_Renderer* renderer);
int SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const void* srcrect, const void* dstrect);
void SDL_RenderPresent(SDL_Renderer* renderer);

SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, uint32_t format, int access, int w, int h);
void SDL_DestroyTexture(SDL_Texture* texture);
int SDL_LockTexture(SDL_Texture* texture, const void* rect, void** pixels, int* pitch);
void SDL_UnlockTexture(SDL_Texture* texture);

// Audio functions
SDL_AudioDeviceID SDL_OpenAudioDevice(const char* device, int iscapture, const void* desired, void* obtained, int allowed_changes);
void SDL_CloseAudioDevice(SDL_AudioDeviceID dev);
void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on);
uint32_t SDL_GetQueuedAudioSize(SDL_AudioDeviceID dev);
int SDL_QueueAudio(SDL_AudioDeviceID dev, const void* data, uint32_t len);

// Timing functions
uint64_t SDL_GetPerformanceFrequency(void);
uint64_t SDL_GetPerformanceCounter(void);
void SDL_Delay(uint32_t ms);

// Event functions
typedef struct SDL_Event {
    uint32_t type;
    // Additional event data would go here
} SDL_Event;

int SDL_PollEvent(SDL_Event* event);

// Memory functions
void* SDL_malloc(size_t size);
void SDL_free(void* memory);

// Other utilities
typedef struct SDL_AudioSpec {
    int freq;
    uint16_t format;
    uint8_t channels;
    uint16_t samples;
    void* callback;
    void* userdata;
} SDL_AudioSpec;

void SDL_memset(void* dst, int c, size_t len);

#endif // SDL_H
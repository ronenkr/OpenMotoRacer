/* The part of SDL2 the port uses, for the ESP32 (sdl_esp32.c implements it on FreeRTOS and
   the board). Names, values and layouts as SDL 2's own where the port depends on them
   (key codes in ranges, the event union's fields). */
#ifndef OMR_SDL_SHIM_H
#define OMR_SDL_SHIM_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t Uint8;
typedef int8_t Sint8;
typedef uint16_t Uint16;
typedef int16_t Sint16;
typedef uint32_t Uint32;
typedef int32_t Sint32;
typedef uint64_t Uint64;
typedef int64_t Sint64;
typedef enum { SDL_FALSE = 0, SDL_TRUE = 1 } SDL_bool;

/* init */
#define SDL_INIT_AUDIO 0x10u
#define SDL_INIT_VIDEO 0x20u
#define SDL_INIT_GAMECONTROLLER 0x2000u
#define SDL_INIT_EVENTS 0x4000u
int SDL_Init(Uint32 flags);
int SDL_InitSubSystem(Uint32 flags);
Uint32 SDL_WasInit(Uint32 flags);
void SDL_Quit(void);
const char *SDL_GetError(void);
int SDL_setenv(const char *name, const char *value, int overwrite);
SDL_bool SDL_SetHint(const char *name, const char *value);
char *SDL_GetBasePath(void);
void SDL_free(void *p);
int SDL_GetSystemRAM(void);

/* time */
Uint64 SDL_GetPerformanceCounter(void);
Uint64 SDL_GetPerformanceFrequency(void);
void SDL_Delay(Uint32 ms);

/* threads */
typedef struct SDL_Thread SDL_Thread;
typedef struct SDL_mutex SDL_mutex;
typedef struct SDL_semaphore SDL_sem;
typedef int (*SDL_ThreadFunction)(void *data);
typedef struct { int value; } SDL_atomic_t;
SDL_Thread *SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data);
void SDL_WaitThread(SDL_Thread *t, int *status);
SDL_mutex *SDL_CreateMutex(void);
int SDL_LockMutex(SDL_mutex *m);
int SDL_UnlockMutex(SDL_mutex *m);
SDL_sem *SDL_CreateSemaphore(Uint32 initial);
int SDL_SemPost(SDL_sem *s);
int SDL_SemWaitTimeout(SDL_sem *s, Uint32 ms);
int SDL_AtomicSet(SDL_atomic_t *a, int v);
int SDL_AtomicGet(SDL_atomic_t *a);

/* video */
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct { int x, y, w, h; } SDL_Rect;
typedef struct SDL_Surface { Uint32 flags; void *format; int w, h, pitch; void *pixels; } SDL_Surface;
#define SDL_WINDOWPOS_CENTERED 0x2FFF0000u
#define SDL_WINDOW_FULLSCREEN_DESKTOP 0x1001u
#define SDL_WINDOW_RESIZABLE 0x20u
#define SDL_RENDERER_PRESENTVSYNC 4u
#define SDL_PIXELFORMAT_RGB565 0x15151002u
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_DISABLE 0
#define SDL_MESSAGEBOX_ERROR 0x10u
#define SDL_MESSAGEBOX_INFORMATION 0x40u
SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags);
void SDL_DestroyWindow(SDL_Window *w);
void SDL_SetWindowSize(SDL_Window *w, int width, int height);
int SDL_SetWindowFullscreen(SDL_Window *w, Uint32 flags);
SDL_Surface *SDL_GetWindowSurface(SDL_Window *w);
int SDL_UpdateWindowSurface(SDL_Window *w);
SDL_Renderer *SDL_CreateRenderer(SDL_Window *w, int index, Uint32 flags);
void SDL_DestroyRenderer(SDL_Renderer *r);
int SDL_RenderSetLogicalSize(SDL_Renderer *r, int w, int h);
SDL_Texture *SDL_CreateTexture(SDL_Renderer *r, Uint32 format, int access, int w, int h);
void SDL_DestroyTexture(SDL_Texture *t);
int SDL_UpdateTexture(SDL_Texture *t, const SDL_Rect *rect, const void *pixels, int pitch);
int SDL_RenderClear(SDL_Renderer *r);
int SDL_RenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *src, const SDL_Rect *dst);
void SDL_RenderPresent(SDL_Renderer *r);
int SDL_ShowCursor(int toggle);
int SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *msg, SDL_Window *w);
SDL_Surface *SDL_CreateRGBSurfaceWithFormatFrom(void *pixels, int w, int h, int depth, int pitch,
                                                Uint32 format);
void SDL_FreeSurface(SDL_Surface *s);
int SDL_SaveBMP(SDL_Surface *s, const char *file);

/* sound */
typedef Uint32 SDL_AudioDeviceID;
typedef Uint16 SDL_AudioFormat;
typedef void (*SDL_AudioCallback)(void *user, Uint8 *stream, int len);
typedef struct {
    int freq;
    SDL_AudioFormat format;
    Uint8 channels, silence;
    Uint16 samples, padding;
    Uint32 size;
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;
#define AUDIO_S16SYS 0x8010
SDL_AudioDeviceID SDL_OpenAudioDevice(const char *dev, int capture, const SDL_AudioSpec *want,
                                      SDL_AudioSpec *got, int changes);
void SDL_PauseAudioDevice(SDL_AudioDeviceID d, int pause);
void SDL_CloseAudioDevice(SDL_AudioDeviceID d);

/* keys */
typedef Sint32 SDL_Keycode;
#define SDLK_SCANCODE_MASK (1 << 30)
#define SDLK_SC(x) ((x) | SDLK_SCANCODE_MASK)
enum {
    SDLK_RETURN = '\r', SDLK_ESCAPE = 0x1b, SDLK_BACKSPACE = '\b', SDLK_TAB = '\t', SDLK_SPACE = ' ',
    SDLK_F1 = SDLK_SC(58), SDLK_F12 = SDLK_SC(69), SDLK_PAUSE = SDLK_SC(72),
    SDLK_INSERT = SDLK_SC(73), SDLK_HOME = SDLK_SC(74), SDLK_PAGEUP = SDLK_SC(75),
    SDLK_END = SDLK_SC(77), SDLK_PAGEDOWN = SDLK_SC(78), SDLK_RIGHT = SDLK_SC(79),
    SDLK_LEFT = SDLK_SC(80), SDLK_DOWN = SDLK_SC(81), SDLK_UP = SDLK_SC(82),
    SDLK_KP_DIVIDE = SDLK_SC(84), SDLK_KP_MULTIPLY = SDLK_SC(85), SDLK_KP_MINUS = SDLK_SC(86),
    SDLK_KP_PLUS = SDLK_SC(87), SDLK_KP_ENTER = SDLK_SC(88), SDLK_KP_1 = SDLK_SC(89),
    SDLK_KP_9 = SDLK_SC(97), SDLK_KP_0 = SDLK_SC(98),
    SDLK_LCTRL = SDLK_SC(224), SDLK_LSHIFT = SDLK_SC(225), SDLK_LALT = SDLK_SC(226),
    SDLK_RCTRL = SDLK_SC(228), SDLK_RSHIFT = SDLK_SC(229), SDLK_RALT = SDLK_SC(230),
};
#define KMOD_ALT 0x0300

/* events */
enum {
    SDL_QUIT = 0x100, SDL_WINDOWEVENT = 0x200, SDL_KEYDOWN = 0x300, SDL_KEYUP, SDL_TEXTINPUT = 0x303,
    SDL_MOUSEMOTION = 0x400, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP,
    SDL_JOYDEVICEADDED = 0x605, SDL_JOYDEVICEREMOVED,
};
enum { SDL_WINDOWEVENT_FOCUS_GAINED = 12, SDL_WINDOWEVENT_FOCUS_LOST = 13 };
#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_RIGHT 3
typedef struct { int scancode; SDL_Keycode sym; Uint16 mod; Uint32 unused; } SDL_Keysym;
typedef union SDL_Event {
    Uint32 type;
    struct { Uint32 type, timestamp, windowID; Uint8 event; } window;
    struct { Uint32 type, timestamp, windowID; Uint8 state, repeat; SDL_Keysym keysym; } key;
    struct { Uint32 type, timestamp, windowID; char text[32]; } text;
    struct { Uint32 type, timestamp, windowID, which, state; Sint32 x, y, xrel, yrel; } motion;
    struct { Uint32 type, timestamp, windowID, which; Uint8 button, state, clicks; Sint32 x, y; } button;
    Uint8 padding[56];
} SDL_Event;
int SDL_PollEvent(SDL_Event *e);

/* joysticks: none on the board (the game's pad is fed through joy_fake_pad) */
typedef struct SDL_Joystick SDL_Joystick;
typedef struct SDL_GameController SDL_GameController;
typedef enum {
    SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK, SDL_CONTROLLER_BUTTON_GUIDE, SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
} SDL_GameControllerButton;
typedef enum {
    SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY, SDL_CONTROLLER_AXIS_RIGHTX,
    SDL_CONTROLLER_AXIS_RIGHTY, SDL_CONTROLLER_AXIS_TRIGGERLEFT, SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
} SDL_GameControllerAxis;
#define SDL_HAT_UP 1
#define SDL_HAT_RIGHT 2
#define SDL_HAT_DOWN 4
#define SDL_HAT_LEFT 8
int SDL_NumJoysticks(void);
SDL_bool SDL_IsGameController(int i);
SDL_GameController *SDL_GameControllerOpen(int i);
void SDL_GameControllerClose(SDL_GameController *c);
SDL_Joystick *SDL_GameControllerGetJoystick(SDL_GameController *c);
void SDL_GameControllerUpdate(void);
Sint16 SDL_GameControllerGetAxis(SDL_GameController *c, SDL_GameControllerAxis a);
Uint8 SDL_GameControllerGetButton(SDL_GameController *c, SDL_GameControllerButton b);
SDL_Joystick *SDL_JoystickOpen(int i);
void SDL_JoystickClose(SDL_Joystick *j);
SDL_bool SDL_JoystickGetAttached(SDL_Joystick *j);
void SDL_JoystickUpdate(void);
int SDL_JoystickNumAxes(SDL_Joystick *j);
int SDL_JoystickNumButtons(SDL_Joystick *j);
int SDL_JoystickNumHats(SDL_Joystick *j);
Sint16 SDL_JoystickGetAxis(SDL_Joystick *j, int a);
Uint8 SDL_JoystickGetButton(SDL_Joystick *j, int b);
Uint8 SDL_JoystickGetHat(SDL_Joystick *j, int h);
int SDL_JoystickRumble(SDL_Joystick *j, Uint16 lo, Uint16 hi, Uint32 ms);

#endif

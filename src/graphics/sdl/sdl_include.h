#ifndef SDL_INCLUDE_H
#define SDL_INCLUDE_H

#include <SDL2/SDL.h>

#include "opengl.h"

#define RENDER_BATCH_SIZE 1024

typedef enum {
    Render_RectangleFlags_Texture = 1 << 0,
    Render_RectangleFlags_MSDF    = 1 << 1,
} Render_RectangleFlags;

typedef struct Render_Rectangle Render_Rectangle;
struct Render_Rectangle {
    V2F32 min;
    V2F32 max;
    V4F32 color;
    V2F32 uv_min;
    V2F32 uv_max;
    U32   flags;
};

typedef struct Render_Batch Render_Batch;
struct Render_Batch {
    Render_Batch *next;
    Render_Batch *previous;

    GLuint texture_id;
    U32 size;
    Render_Rectangle rectangles[RENDER_BATCH_SIZE];
};

typedef struct Render_BatchList Render_BatchList;
struct Render_BatchList {
    Render_Batch *first;
    Render_Batch *last;
};

typedef struct Render_Texture Render_Texture;
struct Render_Texture {
    GLuint texture_id;
    V2U32  size;
};

typedef struct Gfx_Context Gfx_Context;
struct Gfx_Context {
    Arena            arena;
    Arena_Temporary  frame_restore;
    SDL_Window      *window;
    SDL_GLContext    gl_context;

    Render_BatchList batches;
    GLuint           program;
    GLuint           vao;
    GLuint           vbo;
    GLint            uniform_projection_location;
    GLint            uniform_sampler_location;
};

typedef enum {
    Gfx_EventKind_Null,
    Gfx_EventKind_Quit,
    Gfx_EventKind_KeyPress,
    Gfx_EventKind_KeyRelease,
    Gfx_EventKind_Text,
    Gfx_EventKind_Scroll,
    Gfx_EventKind_Resize,

    Gfx_EventKind_COUNT,
} Gfx_EventKind;

typedef enum {
    Gfx_KeyModifier_Control = 1 << 0,
    Gfx_KeyModifier_Shift   = 1 << 1,
} Gfx_KeyModifier;

typedef enum {
    Gfx_Key_Null,

    Gfx_Key_A,
    Gfx_Key_B,
    Gfx_Key_C,
    Gfx_Key_D,
    Gfx_Key_E,
    Gfx_Key_F,
    Gfx_Key_G,
    Gfx_Key_H,
    Gfx_Key_I,
    Gfx_Key_J,
    Gfx_Key_K,
    Gfx_Key_L,
    Gfx_Key_M,
    Gfx_Key_N,
    Gfx_Key_O,
    Gfx_Key_P,
    Gfx_Key_Q,
    Gfx_Key_R,
    Gfx_Key_S,
    Gfx_Key_T,
    Gfx_Key_U,
    Gfx_Key_V,
    Gfx_Key_W,
    Gfx_Key_X,
    Gfx_Key_Y,
    Gfx_Key_Z,

    Gfx_Key_0,
    Gfx_Key_1,
    Gfx_Key_2,
    Gfx_Key_3,
    Gfx_Key_4,
    Gfx_Key_5,
    Gfx_Key_6,
    Gfx_Key_7,
    Gfx_Key_8,
    Gfx_Key_9,

    Gfx_Key_F1,
    Gfx_Key_F2,
    Gfx_Key_F3,
    Gfx_Key_F4,
    Gfx_Key_F5,
    Gfx_Key_F6,
    Gfx_Key_F7,
    Gfx_Key_F8,
    Gfx_Key_F9,
    Gfx_Key_F10,
    Gfx_Key_F11,
    Gfx_Key_F12,

    Gfx_Key_Backspace,
    Gfx_Key_Space,
    Gfx_Key_Alt,
    Gfx_Key_OS,
    Gfx_Key_Tab,
    Gfx_Key_Return,
    Gfx_Key_Shift,
    Gfx_Key_Control,
    Gfx_Key_Escape,
    Gfx_Key_PageUp,
    Gfx_Key_PageDown,
    Gfx_Key_End,
    Gfx_Key_Home,
    Gfx_Key_Left,
    Gfx_Key_Right,
    Gfx_Key_Up,
    Gfx_Key_Down,
    Gfx_Key_Delete,
    Gfx_Key_MouseLeft,
    Gfx_Key_MouseRight,
    Gfx_Key_MouseMiddle,
    Gfx_Key_MouseLeftDouble,
    Gfx_Key_MouseRightDouble,
    Gfx_Key_MouseMiddleDouble,
} Gfx_Key;

typedef struct Gfx_Event Gfx_Event;
struct Gfx_Event {
    Gfx_Event *next;
    Gfx_Event *previous;

    Gfx_EventKind   kind;
    Gfx_Key         key;
    Gfx_KeyModifier key_modifiers;
    V2F32           scroll;
};

typedef struct Gfx_EventList Gfx_EventList;
struct Gfx_EventList {
    Gfx_Event *first;
    Gfx_Event *last;
};

internal Gfx_EventList gfx_get_events(Arena *arena, Gfx_Context *gfx);
internal V2F32 gfx_get_mouse_position(Gfx_Context *gfx);

internal Void render_begin(Gfx_Context *gfx);
internal Void render_end(Gfx_Context *gfx);

typedef struct Render_RectangleParams Render_RectangleParams;
struct Render_RectangleParams {
    V2F32                 min;
    V2F32                 max;
    V4F32                 color;
    V2F32                 uv_min;
    V2F32                 uv_max;
    Render_Texture        texture;
    Render_RectangleFlags flags;
};
#define render_rectangle(gfx, minimum, maximum, ...) render_rectangle_internal(gfx, &(Render_RectangleParams) { .min = minimum, .max = maximum, .color = v4f32(1.0f, 1.0f, 1.0f, 1.0f), __VA_ARGS__ })
internal Void render_rectangle_internal(Gfx_Context *gfx, Render_RectangleParams *parameters);

#endif // SDL_INCLUDE_H

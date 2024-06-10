#ifndef WIN32_GRAPHICS_INCLUDE_H
#define WIN32_GRAPHICS_INCLUDE_H

#define UNICODE
#define WIN32_LEAN_AND_MEAN

#pragma warning(push, 0)
#include <Windows.h>
#pragma warning(pop)

typedef struct Gfx_Context Gfx_Context;
struct Gfx_Context {
    HWND hwnd;
    HDC  hdc;
};

global Gfx_Key win32_key_table[128] = {
    [0x30]       = Gfx_Key_0,
    [0x31]       = Gfx_Key_1,
    [0x32]       = Gfx_Key_2,
    [0x33]       = Gfx_Key_3,
    [0x34]       = Gfx_Key_4,
    [0x35]       = Gfx_Key_5,
    [0x36]       = Gfx_Key_6,
    [0x37]       = Gfx_Key_7,
    [0x38]       = Gfx_Key_8,
    [0x39]       = Gfx_Key_9,
    [0x41]       = Gfx_Key_A,
    [0x42]       = Gfx_Key_B,
    [0x43]       = Gfx_Key_C,
    [0x44]       = Gfx_Key_D,
    [0x45]       = Gfx_Key_E,
    [0x46]       = Gfx_Key_F,
    [0x47]       = Gfx_Key_G,
    [0x48]       = Gfx_Key_H,
    [0x49]       = Gfx_Key_I,
    [0x4A]       = Gfx_Key_J,
    [0x4B]       = Gfx_Key_K,
    [0x4C]       = Gfx_Key_L,
    [0x4D]       = Gfx_Key_M,
    [0x4E]       = Gfx_Key_N,
    [0x4F]       = Gfx_Key_O,
    [0x50]       = Gfx_Key_P,
    [0x51]       = Gfx_Key_Q,
    [0x52]       = Gfx_Key_R,
    [0x53]       = Gfx_Key_S,
    [0x54]       = Gfx_Key_T,
    [0x55]       = Gfx_Key_U,
    [0x56]       = Gfx_Key_V,
    [0x57]       = Gfx_Key_W,
    [0x58]       = Gfx_Key_X,
    [0x59]       = Gfx_Key_Y,
    [0x5A]       = Gfx_Key_Z,
    [VK_F1]      = Gfx_Key_F1,
    [VK_F2]      = Gfx_Key_F2,
    [VK_F3]      = Gfx_Key_F3,
    [VK_F4]      = Gfx_Key_F4,
    [VK_F5]      = Gfx_Key_F5,
    [VK_F6]      = Gfx_Key_F6,
    [VK_F7]      = Gfx_Key_F7,
    [VK_F8]      = Gfx_Key_F8,
    [VK_F9]      = Gfx_Key_F9,
    [VK_F10]     = Gfx_Key_F10,
    [VK_F11]     = Gfx_Key_F11,
    [VK_F12]     = Gfx_Key_F12,
    [VK_BACK]    = Gfx_Key_Backspace,
    [VK_SPACE]   = Gfx_Key_Space,
    [VK_MENU]    = Gfx_Key_Alt,
    [VK_LWIN]    = Gfx_Key_OS,
    [VK_RWIN]    = Gfx_Key_OS,
    [VK_TAB]     = Gfx_Key_Tab,
    [VK_RETURN]  = Gfx_Key_Return,
    [VK_SHIFT]   = Gfx_Key_Shift,
    [VK_CONTROL] = Gfx_Key_Control,
    [VK_ESCAPE]  = Gfx_Key_Escape,
    [VK_PRIOR]   = Gfx_Key_PageUp,
    [VK_NEXT]    = Gfx_Key_PageDown,
    [VK_END]     = Gfx_Key_End,
    [VK_HOME]    = Gfx_Key_Home,
    [VK_LEFT]    = Gfx_Key_Left,
    [VK_RIGHT]   = Gfx_Key_Right,
    [VK_UP]      = Gfx_Key_Up,
    [VK_DOWN]    = Gfx_Key_Down,
    [VK_DELETE]  = Gfx_Key_Delete,
    [VK_LBUTTON] = Gfx_Key_MouseLeft,
    [VK_RBUTTON] = Gfx_Key_MouseRight,
    [VK_MBUTTON] = Gfx_Key_MouseMiddle,
};

#endif // WIN32_GRAPHICS_INCLUDE_H

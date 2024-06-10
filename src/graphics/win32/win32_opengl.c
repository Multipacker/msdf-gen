global PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB;
global PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
global PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT;

internal Void win32_get_wgl_functions(Void) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    HWND dummy = CreateWindowEx(
        0,
        cstr16_from_str8(scratch.arena, str8_literal("STATIC")),
        cstr16_from_str8(scratch.arena, str8_literal("DummyWindow")),
        WS_OVERLAPPED,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, NULL, NULL
    );

    if (dummy) {
        HDC dc = GetDC(dummy);

        if (dc) {
            PIXELFORMATDESCRIPTOR desc = {
                .nSize = sizeof(desc),
                .nVersion = 1,
                .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                .iPixelType = PFD_TYPE_RGBA,
                .cColorBits = 24,
            };

            S32 format = ChoosePixelFormat(dc, &desc);

            if (format) {
                if (DescribePixelFormat(dc, format, sizeof(desc), &desc)) {
                    if (SetPixelFormat(dc, format, &desc)) {
                        HGLRC rc = wglCreateContext(dc);
                        if (rc) {
                            wglMakeCurrent(dc, rc);

                            Str8 extensions = { 0 };

                            PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (Void*) wglGetProcAddress("wglGetExtensionsStringARB");
                            if (wglGetExtensionsStringARB) {
                                extensions = str8_cstr((CStr) wglGetExtensionsStringARB(dc));
                            }

                            Str8List extension_list = str8_split_by_codepoints(scratch.arena, extensions, str8_literal(" "));

                            for (Str8Node *node = extension_list.first; node; node = node->next) {
                                Str8 string = node->string;

                                if (str8_equal(string, str8_literal("WGL_ARB_pixel_format"))) {
                                    wglChoosePixelFormatARB = (Void *) wglGetProcAddress("wglChoosePixelFormatARB");
                                } else if (str8_equal(string, str8_literal("WGL_ARB_create_context"))) {
                                    wglCreateContextAttribsARB = (Void *) wglGetProcAddress("wglCreateContextAttribsARB");
                                } else if (str8_equal(string, str8_literal("WGL_EXT_swap_control"))) {
                                    wglSwapIntervalEXT = (Void *) wglGetProcAddress("wglSwapIntervalEXT");
                                }
                            }

                            if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB || !wglSwapIntervalEXT) {
                                // "OpenGL does not support required WGL extensions for modern context!
                            }

                            wglMakeCurrent(0, 0);
                            wglDeleteContext(rc);
                        } else {
                            // Failed to create OpenGL context for dummy window
                        }
                    } else {
                        // Cannot set OpenGL pixel format for dummy window!
                    }
                } else {
                    // Failed to describe OpenGL pixel format
                }
            } else {
                // Cannot choose OpenGL pixel format for dummy window!
            }

            ReleaseDC(dummy, dc);
        } else {
            // Failed to get device context for dummy window
        }

        DestroyWindow(dummy);
    } else {
        // Failed to create dummy window
    }

    arena_end_temporary(scratch);
}

internal Void win32_init_opengl(Gfx_Context *gfx) {
    win32_get_wgl_functions();

    S32 pixel_attrib[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 24,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
        0,
    };

    S32 format = 0;
    UINT formats = 0;
    if (wglChoosePixelFormatARB(gfx->hdc, pixel_attrib, 0, 1, &format, &formats) || formats == 0) {
        PIXELFORMATDESCRIPTOR desc = { .nSize = sizeof(desc) };
        if (DescribePixelFormat(gfx->hdc, format, sizeof(desc), &desc)) {
            if (SetPixelFormat(gfx->hdc, format, &desc)) {
                S32 context_attrib[] = {
                    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                    WGL_CONTEXT_MINOR_VERSION_ARB, 5,
                    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                    0,
                };

                HGLRC rc = wglCreateContextAttribsARB(gfx->hdc, NULL, context_attrib);
                if (rc) {
                    wglMakeCurrent(gfx->hdc, rc);

#define X(type, name) name = (type) wglGetProcAddress(#name); assert(name);
                    GL_FUNCTIONS(X)
#undef X
                } else {
                    // Cannot create modern OpenGL context! OpenGL version 4.5 not supported?
                }
            } else {
                // Cannot set OpenGL selected pixel format!
            }
        } else {
            // Failed to describe OpenGL pixel format
        }

    } else {
        // OpenGL does not support required pixel format!
    }
}

internal Void gfx_swap_buffers(Gfx_Context *gfx) {
    SwapBuffers(gfx->hdc);
}

#ifndef OPENGL_H
#define OPENGL_H

// Definitions taken from:
//   * https://github.com/KhronosGroup/OpenGL-Registry
//   * https://registry.khronos.org/OpenGL-Refpages/gl4/

#define GL_ARRAY_BUFFER         0x8892
#define GL_BACK                 0x0405
#define GL_BLEND                0x0BE2
#define GL_CLAMP_TO_EDGE        0x812F
#define GL_COLOR_BUFFER_BIT     0x00004000
#define GL_DYNAMIC_DRAW         0x88E8
#define GL_FALSE                0
#define GL_FLOAT                0x1406
#define GL_FRAGMENT_SHADER      0x8B30
#define GL_FRAMEBUFFER_SRGB     0x8DB9
#define GL_INFO_LOG_LENGTH      0x8B84
#define GL_INT                  0x1404
#define GL_LINEAR               0x2601
#define GL_NEAREST              0x2600
#define GL_ONE                  1
#define GL_ONE_MINUS_SRC1_COLOR 0x88FA
#define GL_ONE_MINUS_SRC_ALPHA  0x0303
#define GL_R8                   0x8229
#define GL_RED                  0x1903
#define GL_RGBA                 0x1908
#define GL_RGBA8                0x8058
#define GL_SCISSOR_TEST         0x0C11
#define GL_SRC1_ALPHA           0x8589
#define GL_SRC1_COLOR           0x88F9
#define GL_SRC_ALPHA            0x0302
#define GL_SRGB8_ALPHA8         0x8C43
#define GL_STREAM_DRAW          0x88E0
#define GL_TEXTURE_2D           0x0DE1
#define GL_TEXTURE_MAG_FILTER   0x2800
#define GL_TEXTURE_MIN_FILTER   0x2801
#define GL_TEXTURE_WRAP_S       0x2802
#define GL_TEXTURE_WRAP_T       0x2803
#define GL_TRIANGLE_STRIP       0x0005
#define GL_TRUE                 1
#define GL_UNPACK_ALIGNMENT     0x0CF5
#define GL_UNSIGNED_BYTE        0x1401
#define GL_UNSIGNED_INT         0x1405
#define GL_VERTEX_SHADER        0x8B31
#define GL_WRITE_ONLY           0x88B9

typedef char            GLchar;
typedef float           GLfloat;
typedef int             GLint;
typedef intptr_t        GLintptr;
typedef int             GLsizei;
typedef unsigned int    GLbitfield;
typedef unsigned int    GLboolean;
typedef unsigned int    GLenum;
typedef unsigned int    GLuint;
typedef signed long int GLsizeiptr;

typedef GLboolean (*PFNGLUNMAPBUFFERPROC)(GLenum target);
typedef GLint     (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef GLuint    (*PFNGLCREATEPROGRAMPROC)(Void);
typedef GLuint    (*PFNGLCREATESHADERPROC)(GLenum shaderType);
typedef Void      (*PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef Void      (*PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef Void      (*PFNGLBINDSAMPLERPROC)(GLuint unit, GLuint sampler);
typedef Void      (*PFNGLBINDTEXTUREPROC)(GLenum target, GLuint texture);
typedef Void      (*PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef Void      (*PFNGLBLENDFUNCSEPARATEPROC)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
typedef Void      (*PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const Void *data, GLenum usage);
typedef Void      (*PFNGLCLEARCOLORPROC)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef Void      (*PFNGLCLEARPROC)(GLbitfield mask);
typedef Void      (*PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef Void      (*PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef Void      (*PFNGLDELETETEXTURESPROC)(GLsizei n, const GLuint *textures);
typedef Void      (*PFNGLDETACHSHADERPROC)(GLuint program, GLuint shader);
typedef Void      (*PFNGLDISABLEPROC)(GLenum cap);
typedef Void      (*PFNGLDRAWARRAYSINSTANCEDPROC)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
typedef Void      (*PFNGLDRAWBUFFERPROC)(GLenum buf);
typedef Void      (*PFNGLENABLEPROC)(GLenum cap);
typedef Void      (*PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef Void      (*PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef Void      (*PFNGLGENSAMPLERSPROC)(GLsizei n, GLuint *samplers);
typedef Void      (*PFNGLGENTEXTURESPROC)(GLsizei n, GLuint *textures);
typedef Void      (*PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef Void      (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef Void      (*PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef Void      (*PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef Void      (*PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef Void      (*PFNGLLINKPROGRAMPROC)(GLuint program);
typedef Void      (*PFNGLPIXELSTOREI)(GLenum pname, GLint param);
typedef Void      (*PFNGLSAMPLERPARAMETERIPROC)(GLuint sampler, GLenum pname, GLint param);
typedef Void      (*PFNGLSCISSORPROC)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef Void      (*PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
typedef Void      (*PFNGLTEXIMAGE2DPROC)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const Void *data);
typedef Void      (*PFNGLTEXSUBIMAGE2DPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const Void *pixels);
typedef Void      (*PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef Void      (*PFNGLUNIFORM1IVPROC)(GLint location, GLsizei count, const GLint *value);
typedef Void      (*PFNGLUNIFORMMATRIX3FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef Void      (*PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef Void      (*PFNGLUSEPROGRAMPROC)(GLuint program);
typedef Void      (*PFNGLVALIDATEPROGRAMPROC)(GLuint program);
typedef Void      (*PFNGLVERTEXATTRIBDIVISOR)(GLuint index, GLuint divisor);
typedef Void      (*PFNGLVERTEXATTRIBIPOINTER)(GLuint index, GLint size, GLenum type, GLsizei stride, const Void *pointer);
typedef Void      (*PFNGLVERTEXATTRIBPOINTER)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const Void *pointer);
typedef Void      (*PFNGLVIEWPORTPROC)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef Void     *(*PFNGLMAPBUFFERPROC)(GLenum target, GLenum access);

#define GL_FUNCTIONS(X)                                                        \
X(PFNGLATTACHSHADERPROC,                    glAttachShader)                    \
X(PFNGLBINDBUFFERPROC,                      glBindBuffer)                      \
X(PFNGLBINDSAMPLERPROC,                     glBindSampler)                     \
X(PFNGLBINDTEXTUREPROC,                     glBindTexture)                     \
X(PFNGLBINDVERTEXARRAYPROC,                 glBindVertexArray)                 \
X(PFNGLBLENDFUNCSEPARATEPROC,               glBlendFuncSeparate)               \
X(PFNGLBUFFERDATAPROC,                      glBufferData)                      \
X(PFNGLCLEARCOLORPROC,                      glClearColor)                      \
X(PFNGLCLEARPROC,                           glClear)                           \
X(PFNGLCOMPILESHADERPROC,                   glCompileShader)                   \
X(PFNGLCREATEPROGRAMPROC,                   glCreateProgram)                   \
X(PFNGLCREATESHADERPROC,                    glCreateShader)                    \
X(PFNGLDELETEBUFFERSPROC,                   glDeleteBuffers)                   \
X(PFNGLDELETETEXTURESPROC,                  glDeleteTextures)                  \
X(PFNGLDETACHSHADERPROC,                    glDetachShader)                    \
X(PFNGLDISABLEPROC,                         glDisable)                         \
X(PFNGLDRAWARRAYSINSTANCEDPROC,             glDrawArraysInstanced)             \
X(PFNGLDRAWBUFFERPROC,                      glDrawBuffer)                      \
X(PFNGLENABLEPROC,                          glEnable)                          \
X(PFNGLENABLEVERTEXATTRIBARRAYPROC,         glEnableVertexAttribArray)         \
X(PFNGLGENBUFFERSPROC,                      glGenBuffers)                      \
X(PFNGLGENSAMPLERSPROC,                     glGenSamplers)                     \
X(PFNGLGENTEXTURESPROC,                     glGenTextures)                     \
X(PFNGLGENVERTEXARRAYSPROC,                 glGenVertexArrays)                 \
X(PFNGLGETPROGRAMINFOLOGPROC,               glGetProgramInfoLog)               \
X(PFNGLGETPROGRAMIVPROC,                    glGetProgramiv)                    \
X(PFNGLGETSHADERINFOLOGPROC,                glGetShaderInfoLog)                \
X(PFNGLGETSHADERIVPROC,                     glGetShaderiv)                     \
X(PFNGLGETUNIFORMLOCATIONPROC,              glGetUniformLocation)              \
X(PFNGLLINKPROGRAMPROC,                     glLinkProgram)                     \
X(PFNGLMAPBUFFERPROC,                       glMapBuffer)                       \
X(PFNGLPIXELSTOREI,                         glPixelStorei)                     \
X(PFNGLSAMPLERPARAMETERIPROC,               glSamplerParameteri)               \
X(PFNGLSCISSORPROC,                         glScissor)                         \
X(PFNGLSHADERSOURCEPROC,                    glShaderSource)                    \
X(PFNGLTEXIMAGE2DPROC,                      glTexImage2D)                      \
X(PFNGLTEXSUBIMAGE2DPROC,                   glTexSubImage2D)                   \
X(PFNGLUNIFORM1IPROC,                       glUniform1i)                       \
X(PFNGLUNIFORM1IVPROC,                      glUniform1iv)                      \
X(PFNGLUNIFORMMATRIX3FVPROC,                glUniformMatrix3fv)                \
X(PFNGLUNIFORMMATRIX4FVPROC,                glUniformMatrix4fv)                \
X(PFNGLUNMAPBUFFERPROC,                     glUnmapBuffer)                     \
X(PFNGLUSEPROGRAMPROC,                      glUseProgram)                      \
X(PFNGLVALIDATEPROGRAMPROC,                 glValidateProgram)                 \
X(PFNGLVERTEXATTRIBDIVISOR,                 glVertexAttribDivisor)             \
X(PFNGLVERTEXATTRIBIPOINTER,                glVertexAttribIPointer)            \
X(PFNGLVERTEXATTRIBPOINTER,                 glVertexAttribPointer)             \
X(PFNGLVIEWPORTPROC,                        glViewport)

#define X(type, name) global type name;
GL_FUNCTIONS(X)
#undef X

#endif // OPENGL_H

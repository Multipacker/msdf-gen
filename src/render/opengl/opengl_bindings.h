#ifndef OPENGL_H
#define OPENGL_H

// Definitions taken from:
//   * https://github.com/KhronosGroup/OpenGL-Registry
//   * https://registry.khronos.org/OpenGL-Refpages/gl4/

#define GL_BLEND                0x0BE2
#define GL_CLAMP_TO_EDGE        0x812F
#define GL_COLOR_BUFFER_BIT     0x00004000
#define GL_COMPILE_STATUS       0x8B81
#define GL_DYNAMIC_DRAW         0x88E8
#define GL_FALSE                0
#define GL_FLOAT                0x1406
#define GL_FRAGMENT_SHADER      0x8B30
#define GL_INFO_LOG_LENGTH      0x8B84
#define GL_INT                  0x1404
#define GL_LINEAR               0x2601
#define GL_LINK_STATUS          0x8B82
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

#define GL_DEBUG_SOURCE_API               0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM     0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER   0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY       0x8249
#define GL_DEBUG_SOURCE_APPLICATION       0x824A
#define GL_DEBUG_SOURCE_OTHER             0x824B

#define GL_DEBUG_TYPE_ERROR               0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR  0x824E
#define GL_DEBUG_TYPE_PORTABILITY         0x824F
#define GL_DEBUG_TYPE_PERFORMANCE         0x8250
#define GL_DEBUG_TYPE_OTHER               0x8251
#define GL_DEBUG_TYPE_PERFORMANCE         0x8250
#define GL_DEBUG_TYPE_OTHER               0x8251
#define GL_DEBUG_TYPE_MARKER              0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP          0x8269
#define GL_DEBUG_TYPE_POP_GROUP           0x826A

#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B

#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#define GL_FRAMEBUFFER_SRGB               0x8DB9

typedef char         GLchar;
typedef float        GLfloat;
typedef int          GLint;
typedef intptr_t     GLintptr;
typedef int          GLsizei;
typedef unsigned int GLbitfield;
typedef unsigned int GLboolean;
typedef unsigned int GLenum;
typedef unsigned int GLuint;

#if OS_WINDOWS && ARCH_X64
typedef signed long long int GLsizeiptr;
#else
typedef signed long int GLsizeiptr;
#endif

#if OS_LINUX
typedef Void (*PFNGLCLEARCOLORPROC)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef Void (*PFNGLCLEARPROC)(GLbitfield mask);
typedef Void (*PFNGLDISABLEPROC)(GLenum cap);
typedef Void (*PFNGLENABLEPROC)(GLenum cap);
typedef Void (*PFNGLPIXELSTOREI)(GLenum pname, GLint param);
typedef Void (*PFNGLSCISSORPROC)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef Void (*PFNGLVIEWPORTPROC)(GLint x, GLint y, GLsizei width, GLsizei height);
#endif

typedef Void   (GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

typedef GLboolean (*PFNGLUNMAPNAMEDBUFFER)(GLuint buffer);
typedef GLint     (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef GLuint    (*PFNGLCREATEPROGRAMPROC)(Void);
typedef GLuint    (*PFNGLCREATESHADERPROC)(GLenum shaderType);
typedef Void      (*PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef Void      (*PFNGLBINDSAMPLERPROC)(GLuint unit, GLuint sampler);
typedef Void      (*PFNGLBINDTEXTUREUNITPROC)(GLuint unit, GLuint texture);
typedef Void      (*PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef Void      (*PFNGLBLENDFUNCSEPARATEPROC)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
typedef Void      (*PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef Void      (*PFNGLCREATEBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef Void      (*PFNGLCREATESAMPLERSPROC)(GLsizei n, GLuint *samplers);
typedef Void      (*PFNGLCREATETEXTURESPROC)(GLenum target, GLsizei n, GLuint *textures);
typedef Void      (*PFNGLCREATEVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef Void      (*PFNGLDEBUGMESSAGECALLBACKPROC) (GLDEBUGPROC *callback, const void *userParam);
typedef Void      (*PFNGLDELETEBUFFERS)(GLsizei n, const GLuint *buffers);
typedef Void      (*PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef Void      (*PFNGLDELETESHADERPROC)(GLuint shader);
typedef Void      (*PFNGLDELETETEXTURESPROC)(GLsizei n, const GLuint *textures);
typedef Void      (*PFNGLDETACHSHADERPROC)(GLuint program, GLuint shader);
typedef Void      (*PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
typedef Void      (*PFNGLENABLEVERTEXARRAYATTRIBPROC)(GLuint vaobj, GLuint index);
typedef Void      (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef Void      (*PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef Void      (*PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef Void      (*PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef Void      (*PFNGLLINKPROGRAMPROC)(GLuint program);
typedef Void      (*PFNGLNAMEDBUFFERDATAPROC)(GLuint buffer, GLsizeiptr size, const Void *data, GLenum usage);
typedef Void      (*PFNGLNAMEDBUFFERSUBDATAPROC)(GLuint buffer, GLintptr offset, GLsizeiptr size, const Void *data);
typedef Void      (*PFNGLPROGRAMUNIFORM1IPROC)(GLuint program, GLint location, GLint v0);
typedef Void      (*PFNGLPROGRAMUNIFORM1IVPROC)(GLuint program, GLint location, GLsizei count, const GLint *value);
typedef Void      (*PFNGLPROGRAMUNIFORMMATRIX3FVPROC)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef Void      (*PFNGLPROGRAMUNIFORMMATRIX4FVPROC)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef Void      (*PFNGLSAMPLERPARAMETERIPROC)(GLuint sampler, GLenum pname, GLint param);
typedef Void      (*PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
typedef Void      (*PFNGLTEXTURESTORAGE2DPROC)(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
typedef Void      (*PFNGLTEXTURESUBIMAGE2DPROC)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const Void *pixels);
typedef Void      (*PFNGLUSEPROGRAMPROC)(GLuint program);
typedef Void      (*PFNGLVERTEXARRAYATTRIBBINDINGPROC)(GLuint vaobj, GLuint attribindex, GLuint bindingindex);
typedef Void      (*PFNGLVERTEXARRAYATTRIBFORMATPROC)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
typedef Void      (*PFNGLVERTEXARRAYATTRIBIFORMATPROC)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
typedef Void      (*PFNGLVERTEXARRAYBINDINGDIVISORPROC)(GLuint vaobj, GLuint bindingindex, GLuint divisor);
typedef Void      (*PFNGLVERTEXARRAYVERTEXBUFFERPROC)(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
typedef Void     *(*PFNGLMAPNAMEDBUFFER)(GLuint buffer, GLenum access);

#define GL_LINUX_FUNCTIONS(X) \
X(PFNGLCLEARCOLORPROC,                glClearColor)                \
X(PFNGLCLEARPROC,                     glClear)                     \
X(PFNGLDISABLEPROC,                   glDisable)                   \
X(PFNGLENABLEPROC,                    glEnable)                    \
X(PFNGLPIXELSTOREI,                   glPixelStorei)               \
X(PFNGLSCISSORPROC,                   glScissor)                   \
X(PFNGLVIEWPORTPROC,                  glViewport)

#define GL_FUNCTIONS(X)                                                        \
X(PFNGLATTACHSHADERPROC,                    glAttachShader)                    \
X(PFNGLBINDSAMPLERPROC,                     glBindSampler)                     \
X(PFNGLBINDTEXTUREUNITPROC,                 glBindTextureUnit)                 \
X(PFNGLBINDVERTEXARRAYPROC,                 glBindVertexArray)                 \
X(PFNGLBLENDFUNCSEPARATEPROC,               glBlendFuncSeparate)               \
X(PFNGLCOMPILESHADERPROC,                   glCompileShader)                   \
X(PFNGLCREATEBUFFERSPROC,                   glCreateBuffers)                   \
X(PFNGLCREATEPROGRAMPROC,                   glCreateProgram)                   \
X(PFNGLCREATESAMPLERSPROC,                  glCreateSamplers)                  \
X(PFNGLCREATESHADERPROC,                    glCreateShader)                    \
X(PFNGLCREATETEXTURESPROC,                  glCreateTextures)                  \
X(PFNGLCREATEVERTEXARRAYSPROC,              glCreateVertexArrays)              \
X(PFNGLDEBUGMESSAGECALLBACKPROC,            glDebugMessageCallback)            \
X(PFNGLDELETEBUFFERS,                       glDeleteBuffers)                   \
X(PFNGLDELETEPROGRAMPROC,                   glDeleteProgram)                   \
X(PFNGLDELETESHADERPROC,                    glDeleteShader)                    \
X(PFNGLDELETETEXTURESPROC,                  glDeleteTextures)                  \
X(PFNGLDETACHSHADERPROC,                    glDetachShader)                    \
X(PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC, glDrawArraysInstancedBaseInstance) \
X(PFNGLENABLEVERTEXARRAYATTRIBPROC,         glEnableVertexArrayAttrib)         \
X(PFNGLGETPROGRAMINFOLOGPROC,               glGetProgramInfoLog)               \
X(PFNGLGETPROGRAMIVPROC,                    glGetProgramiv)                    \
X(PFNGLGETSHADERINFOLOGPROC,                glGetShaderInfoLog)                \
X(PFNGLGETSHADERIVPROC,                     glGetShaderiv)                     \
X(PFNGLGETUNIFORMLOCATIONPROC,              glGetUniformLocation)              \
X(PFNGLLINKPROGRAMPROC,                     glLinkProgram)                     \
X(PFNGLMAPNAMEDBUFFER,                      glMapNamedBuffer)                  \
X(PFNGLNAMEDBUFFERDATAPROC,                 glNamedBufferData)                 \
X(PFNGLNAMEDBUFFERSUBDATAPROC,              glNamedBufferSubData)              \
X(PFNGLPROGRAMUNIFORM1IPROC,                glProgramUniform1i)                \
X(PFNGLPROGRAMUNIFORM1IVPROC,               glProgramUniform1iv)               \
X(PFNGLPROGRAMUNIFORMMATRIX3FVPROC,         glProgramUniformMatrix3fv)         \
X(PFNGLPROGRAMUNIFORMMATRIX4FVPROC,         glProgramUniformMatrix4fv)         \
X(PFNGLSAMPLERPARAMETERIPROC,               glSamplerParameteri)               \
X(PFNGLSHADERSOURCEPROC,                    glShaderSource)                    \
X(PFNGLTEXTURESTORAGE2DPROC,                glTextureStorage2D)                \
X(PFNGLTEXTURESUBIMAGE2DPROC,               glTextureSubImage2D)               \
X(PFNGLUNMAPNAMEDBUFFER,                    glUnmapNamedBuffer)                \
X(PFNGLUSEPROGRAMPROC,                      glUseProgram)                      \
X(PFNGLVERTEXARRAYATTRIBBINDINGPROC,        glVertexArrayAttribBinding)        \
X(PFNGLVERTEXARRAYATTRIBFORMATPROC,         glVertexArrayAttribFormat)         \
X(PFNGLVERTEXARRAYATTRIBIFORMATPROC,        glVertexArrayAttribIFormat)        \
X(PFNGLVERTEXARRAYBINDINGDIVISORPROC,       glVertexArrayBindingDivisor)       \
X(PFNGLVERTEXARRAYVERTEXBUFFERPROC,         glVertexArrayVertexBuffer)

#define X(type, name) global type name;

#if OS_LINUX
GL_LINUX_FUNCTIONS(X)
#else
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glClear(GLbitfield mask);
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glDisable(GLenum cap);
void glEnable(GLenum cap);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glPixelStorei(GLenum pname, GLint param);
#endif

GL_FUNCTIONS(X)
#undef X

#endif // OPENGL_H

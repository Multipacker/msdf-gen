#undef embed_file
#define embed_file(...)
extern U8 opengl_vertex_shader_data[2864];
global Str8 opengl_vertex_shader = (Str8) { opengl_vertex_shader_data, 2864, };
extern U8 opengl_fragment_shader_data[3358];
global Str8 opengl_fragment_shader = (Str8) { opengl_fragment_shader_data, 3358, };

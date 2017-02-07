#include "renderer.h"

static GLuint load_shader(const char* filename, GLenum type)
{
    FILE* f = fopen(filename, "r");
    int len;
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    GLchar source[len+1];
    const GLchar* source_ptr = source;
    fseek(f, 0, SEEK_SET);
    fread(source, 1, len, f);
    source[len] = '\0';
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &source_ptr, NULL);
    glCompileShader(s);
    GLint status;
    glGetShaderiv(s, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
        char buffer[512];
        glGetShaderInfoLog(s, 512, NULL, buffer);
        printf("Shader compilation log:\n%s\n", buffer);
        return 0;
    }
    return s;
}

static void shader_program(GLuint* vert, GLuint* frag, GLuint* prog,
                           const char* vert_filename, const char* frag_filename)
{
    *vert = load_shader(vert_filename, GL_VERTEX_SHADER);
    *frag = load_shader(frag_filename, GL_FRAGMENT_SHADER);
    if(!(*vert) || !(*frag)) return;
    *prog = glCreateProgram();
    glAttachShader(*prog, *vert);
    glAttachShader(*prog, *frag);
    glLinkProgram(*prog);
    GLint link_status;
    glGetProgramiv(*prog, GL_LINK_STATUS, &link_status);
    if(!link_status)
    {
        GLint log_length;
        glGetProgramiv(*prog, GL_INFO_LOG_LENGTH, &log_length);
        char log[log_length];
        glGetProgramInfoLog(*prog, log_length, &log_length, log);
        printf("Shader not linked! Info log:\n%s\n", log);
    }
}

static void shader_get_tex_bank(GLuint prog, shader_tex_bank bank)
{
    bank[0] = glGetUniformLocation(prog, "diffuse_map");
    bank[1] = glGetUniformLocation(prog, "normal_map");
    bank[2] = glGetUniformLocation(prog, "extra_map");
}

vec2 fb_verts[4] = { { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } };
unsigned fb_tris[6] = { 0, 1, 2, 2, 1, 3 };

int pg_renderer_init(struct pg_renderer* rend, vec2 view_size)
{
    /*  Initialize the projection matrix, and the viewport info */
    vec3_set(rend->view_pos, 0, 0, 0);
    vec2_set(rend->view_angle, 0, 0);
    vec2_dup(rend->view_size, view_size);
    mat4_perspective(rend->vert_base.proj_matrix, 1.0f,
                     view_size[0] / view_size[1], 0.1f, 100.0f);
    /*  Initialize the framebuffer objects  */
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &rend->color_buf);
    glBindTexture(GL_TEXTURE_2D, rend->color_buf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, view_size[0], view_size[1], 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &rend->depth_buf);
    glBindRenderbuffer(GL_RENDERBUFFER, rend->depth_buf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                          view_size[0], view_size[1]);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &rend->frame_buf);
    glBindFramebuffer(GL_FRAMEBUFFER, rend->frame_buf);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, rend->color_buf, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, rend->depth_buf);
    GLenum status;
    if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "glCheckFramebufferStatus: error %d", status);
        return 0;
    }
    /*  Set up the verts for drawing the post-process screen quad   */
    glGenBuffers(1, &rend->shader_post.vert_buf);
    glGenBuffers(1, &rend->shader_post.tri_buf);
    glGenVertexArrays(1, &rend->shader_post.vao);
    glBindVertexArray(rend->shader_post.vao);
    glBindBuffer(GL_ARRAY_BUFFER, rend->shader_post.vert_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fb_verts), fb_verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rend->shader_post.tri_buf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(fb_tris), fb_tris, GL_STATIC_DRAW);
    glVertexAttribPointer(rend->shader_2d.attrs.pos, 2, GL_FLOAT, GL_FALSE,
                          sizeof(vec2), 0);
    glEnableVertexAttribArray(rend->shader_2d.attrs.pos);
    /*  Initialize the vertex and fragment shader base uniform buffers  */
    glGenBuffers(1, &rend->vert_base_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, rend->vert_base_buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(rend->vert_base),
                 NULL, GL_DYNAMIC_DRAW);
    glGenBuffers(1, &rend->frag_base_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, rend->frag_base_buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(rend->frag_base),
                 NULL, GL_DYNAMIC_DRAW);
    glGenBuffers(1, &rend->shader_terrain.data_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, rend->shader_terrain.data_buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(rend->shader_terrain.data),
                 NULL, GL_DYNAMIC_DRAW);
    /*  Load the shader sources, compile and link the program, and get all
        the attributes and uniforms */
    shader_program(&rend->shader_post.vert, &rend->shader_post.frag,
                   &rend->shader_post.prog,
                   "src/procgl/post_vert.glsl", "src/procgl/post_frag.glsl");
    rend->shader_post.tex = glGetUniformLocation(rend->shader_post.prog, "tex");
    rend->shader_post.pos = glGetAttribLocation(rend->shader_post.prog, "v_position");

    shader_program(&rend->shader_2d.vert, &rend->shader_2d.frag,
                   &rend->shader_2d.prog,
                   "src/procgl/2d_vert.glsl", "src/procgl/2d_frag.glsl");
    rend->shader_2d.tex = glGetUniformLocation(rend->shader_2d.prog, "tex");
    rend->shader_2d.model_matrix = glGetUniformLocation(rend->shader_2d.prog, "model_matrix");
    rend->shader_2d.attrs.pos = glGetAttribLocation(rend->shader_2d.prog, "v_position");
    rend->shader_2d.attrs.color = glGetAttribLocation(rend->shader_2d.prog, "v_color");
    rend->shader_2d.attrs.tex_coord = glGetAttribLocation(rend->shader_2d.prog, "v_tex_coord");
    rend->shader_2d.attrs.tex_weight = glGetAttribLocation(rend->shader_2d.prog, "v_tex_weight");

    shader_program(&rend->shader_model.vert, &rend->shader_model.frag,
                   &rend->shader_model.prog,
                   "src/procgl/model_vert.glsl",
                   "src/procgl/model_frag.glsl");
    shader_get_tex_bank(rend->shader_model.prog, rend->shader_model.tex_slots);
    rend->shader_model.model_matrix = glGetUniformLocation(rend->shader_model.prog, "model_matrix");
    rend->shader_model.attrs.pos = glGetAttribLocation(rend->shader_model.prog, "v_position");
    rend->shader_model.attrs.normal = glGetAttribLocation(rend->shader_model.prog, "v_normal");
    rend->shader_model.attrs.tangent = glGetAttribLocation(rend->shader_model.prog, "v_tangent");
    rend->shader_model.attrs.bitangent = glGetAttribLocation(rend->shader_model.prog, "v_bitangent");
    rend->shader_model.attrs.tex_coord = glGetAttribLocation(rend->shader_model.prog, "v_tex_coord");
    rend->shader_model.vert_base_block =
        glGetUniformBlockIndex(rend->shader_model.prog, "vert_base");
    glUniformBlockBinding(rend->shader_model.prog,
                          rend->shader_model.vert_base_block, 0);
    rend->shader_model.frag_base_block =
        glGetUniformBlockIndex(rend->shader_model.prog, "frag_base");
    glUniformBlockBinding(rend->shader_model.prog,
                          rend->shader_model.frag_base_block, 1);

    shader_program(&rend->shader_terrain.vert, &rend->shader_terrain.frag,
                   &rend->shader_terrain.prog,
                   "src/procgl/terrain_vert.glsl",
                   "src/procgl/terrain_frag.glsl");
    shader_get_tex_bank(rend->shader_terrain.prog, rend->shader_terrain.tex_slots);
    rend->shader_terrain.model_matrix = glGetUniformLocation(rend->shader_terrain.prog, "model_matrix");
    rend->shader_terrain.attrs.pos = glGetAttribLocation(rend->shader_terrain.prog, "v_position");
    rend->shader_terrain.attrs.normal = glGetAttribLocation(rend->shader_terrain.prog, "v_normal");
    rend->shader_terrain.attrs.tangent = glGetAttribLocation(rend->shader_terrain.prog, "v_tangent");
    rend->shader_terrain.attrs.bitangent = glGetAttribLocation(rend->shader_terrain.prog, "v_bitangent");
    rend->shader_terrain.attrs.tex_coord = glGetAttribLocation(rend->shader_terrain.prog, "v_tex_coord");
    rend->shader_terrain.vert_base_block =
        glGetUniformBlockIndex(rend->shader_terrain.prog, "vert_base");
    glUniformBlockBinding(rend->shader_terrain.prog,
                          rend->shader_terrain.vert_base_block, 0);
    rend->shader_terrain.frag_base_block =
        glGetUniformBlockIndex(rend->shader_terrain.prog, "frag_base");
    glUniformBlockBinding(rend->shader_terrain.prog,
                          rend->shader_terrain.frag_base_block, 1);
    rend->shader_terrain.data_block =
        glGetUniformBlockIndex(rend->shader_terrain.prog, "terrain_data");
    glUniformBlockBinding(rend->shader_terrain.prog,
                          rend->shader_terrain.data_block, 2);
    return 1;
}

static void build_projection(struct pg_renderer* rend)
{
    mat4_look_at(rend->vert_base.view_matrix,
                 (vec3){ 0.0f, 0.0f, 0.0f },
                 (vec3){ 0.0f, 1.0f, 0.0f },
                 (vec3){ 0.0f, 0.0f, 1.0f });
    mat4_rotate(rend->vert_base.view_matrix, rend->vert_base.view_matrix,
                1, 0, 0, rend->view_angle[1]);
    mat4_rotate(rend->vert_base.view_matrix, rend->vert_base.view_matrix,
                0, 0, 1, rend->view_angle[0]);
    mat4_translate_in_place(rend->vert_base.view_matrix, -rend->view_pos[0],
                            -rend->view_pos[1], -rend->view_pos[2]);
    mat4_mul(rend->vert_base.projview_matrix, rend->vert_base.proj_matrix,
             rend->vert_base.view_matrix);
    /*  Next: pg_renderer_begin_*() */
}

static void update_shader_base(struct pg_renderer* rend)
{
    glBindBuffer(GL_UNIFORM_BUFFER, rend->vert_base_buffer);
    GLvoid* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    memcpy(ptr, &rend->vert_base, sizeof(rend->vert_base));
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, rend->frag_base_buffer);
    ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    memcpy(ptr, &rend->frag_base, sizeof(rend->frag_base));
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, rend->vert_base_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, rend->frag_base_buffer);
    glClearColor(rend->frag_base.fog_color[0], rend->frag_base.fog_color[1],
                 rend->frag_base.fog_color[2], 1.0f);
}

static void load_textures(shader_tex_bank sh_slot, buffer_tex_bank tex_slot)
{
    GLint tex_uniform[3][4] =
        { { 0, 3, 6, 9 }, { 1, 4, 7, 10 }, { 2, 5, 8, 11 } };
    int i;
    for(i = 0; i < 3; ++i) {
        glUniform1iv(sh_slot[i], 4, tex_uniform[i]);
    }
}

void pg_renderer_begin_2d(struct pg_renderer* rend)
{
    glUseProgram(rend->shader_2d.prog);
}

void pg_renderer_begin_model(struct pg_renderer* rend)
{
    build_projection(rend);
    glUseProgram(rend->shader_model.prog);
    update_shader_base(rend);
    load_textures(rend->shader_model.tex_slots, rend->tex_slots);
}

void pg_renderer_begin_terrain(struct pg_renderer* rend)
{
    build_projection(rend);
    glUseProgram(rend->shader_terrain.prog);
    update_shader_base(rend);
    glBindBuffer(GL_UNIFORM_BUFFER, rend->shader_terrain.data_buffer);
    GLvoid* data_ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    memcpy(data_ptr, &rend->shader_terrain.data,
           sizeof(rend->shader_terrain.data));
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, rend->shader_terrain.data_buffer);
    load_textures(rend->shader_terrain.tex_slots, rend->tex_slots);
}

void pg_renderer_finish(struct pg_renderer* rend)
{
    glUseProgram(rend->shader_post.prog);
    glUniform1i(rend->shader_post.tex, 16);
    glActiveTexture(GL_TEXTURE0 + 16);
    glBindTexture(GL_TEXTURE_2D, rend->color_buf);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(rend->shader_post.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, rend->frame_buf);
}

void pg_renderer_deinit(struct pg_renderer* rend)
{
    glDeleteShader(rend->shader_model.vert);
    glDeleteShader(rend->shader_model.frag);
    glDeleteProgram(rend->shader_model.prog);
    glDeleteShader(rend->shader_terrain.vert);
    glDeleteShader(rend->shader_terrain.frag);
    glDeleteProgram(rend->shader_terrain.prog);
}

void pg_renderer_set_view(struct pg_renderer* rend,
                              vec3 pos, vec2 angle)
{
    if(pos) vec3_dup(rend->view_pos, pos);
    if(angle) vec2_dup(rend->view_angle, angle);
}

void pg_renderer_set_sun(struct pg_renderer* rend,
                             vec3 dir, vec3 color, vec3 amb_color)
{
    if(dir) vec3_dup(rend->frag_base.sun_dir, dir);
    if(color) vec3_dup(rend->frag_base.sun_color, color);
    if(amb_color) vec3_dup(rend->frag_base.amb_color, amb_color);
}

void pg_renderer_set_fog(struct pg_renderer* rend,
                             vec2 plane, vec3 color)
{
    if(plane) vec2_dup(rend->frag_base.fog_plane, plane);
    if(color) vec3_dup(rend->frag_base.fog_color, color);
}

void pg_renderer_get_view(struct pg_renderer* rend,
                              vec3 pos, vec2 angle)
{
    if(pos) vec3_dup(pos, rend->view_pos);
    if(angle) vec2_dup(angle, rend->view_angle);
}

void pg_renderer_get_sun(struct pg_renderer* rend,
                             vec3 dir, vec3 color, vec3 amb_color)
{
    if(dir) vec3_dup(dir, rend->frag_base.sun_dir);
    if(color) vec3_dup(color, rend->frag_base.sun_color);
    if(amb_color) vec3_dup(amb_color, rend->frag_base.amb_color);
}

void pg_renderer_get_fog(struct pg_renderer* rend,
                             vec2 plane, vec3 color)
{
    if(plane) vec2_dup(plane, rend->frag_base.fog_plane);
    if(color) vec3_dup(color, rend->frag_base.fog_color);
}

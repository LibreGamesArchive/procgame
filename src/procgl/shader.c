#include <stdio.h>
#include <GL/glew.h>
#include "ext/linmath.h"
#include "bitwise.h"
#include "arr.h"
#include "viewer.h"
#include "model.h"
#include "shader.h"

/*  Shadow state for the currently used OpenGL shader   */
static struct pg_shader* pg_active_shader;

/*  Code for loading shaders dumped to headers  */
static GLuint compile_glsl_static(const char* src, int len, GLenum type)
{
    /*  Create a shader and give it the source  */
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, &len);
    glCompileShader(shader);
    /*  Print the info log if there were any errors */
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
        GLint log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        char buffer[log_len + 1];
        glGetShaderInfoLog(shader, log_len + 1, NULL, buffer);
        printf("Error loading shader: (static)\nShader compilation log:\n%s",
               buffer);
        return 0;
    }
    return shader;
}


int pg_compile_glsl_static(GLuint* vert, GLuint* frag, GLuint* prog,
                           const char* vert_src, int vert_len,
                           const char* frag_src, int frag_len)
{
    *vert = compile_glsl_static(vert_src, vert_len, GL_VERTEX_SHADER);
    *frag = compile_glsl_static(frag_src, frag_len, GL_FRAGMENT_SHADER);
    if(!(*vert) || !(*frag)) return 0;
    *prog = glCreateProgram();
    glAttachShader(*prog, *vert);
    glAttachShader(*prog, *frag);
    glLinkProgram(*prog);
    /*  Print the info log if there were any errors */
    GLint link_status;
    glGetProgramiv(*prog, GL_LINK_STATUS, &link_status);
    if(!link_status) {
        GLint log_length;
        glGetProgramiv(*prog, GL_INFO_LOG_LENGTH, &log_length);
        GLchar log[log_length];
        glGetProgramInfoLog(*prog, log_length, &log_length, log);
        printf("Shader program info log:\n%s\n", log);
        return 0;
    }
    return 1;
}

int pg_shader_load_static(struct pg_shader* shader,
                          const char* vert, int vert_len,
                          const char* frag, int frag_len)
{
    *shader = (struct pg_shader){
        .mat_idx = { -1, -1, -1, -1, -1, -1, -1 },
        .component_idx = { -1, -1, -1, -1, -1, -1, -1, -1 } };
    return pg_compile_glsl_static(&shader->vert, &shader->frag, &shader->prog,
                                  vert, vert_len, frag, frag_len);
}

/*  Code for loading shaders from files dynamically */
static GLuint compile_glsl(const char* filename, GLenum type)
{
    /*  Read the file into a buffer */
    FILE* f = fopen(filename, "r");
    int len;
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    GLchar source[len+1];
    const GLchar* double_ptr = source; /*  GL requires this   */
    fseek(f, 0, SEEK_SET);
    fread(source, 1, len, f);
    source[len] = '\0';
    /*  Create a shader and give it the source  */
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &double_ptr, NULL);
    glCompileShader(shader);
    /*  Print the info log if there were any errors */
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
        GLint log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        char buffer[log_len + 1];
        glGetShaderInfoLog(shader, log_len + 1, NULL, buffer);
        printf("Error loading shader: %s\nShader compilation log:\n%s",
               filename, buffer);
        fclose(f);
        return 0;
    }
    fclose(f);
    return shader;
}

int pg_compile_glsl(GLuint* vert, GLuint* frag, GLuint* prog,
                   const char* vert_filename, const char* frag_filename)
{
    *vert = compile_glsl(vert_filename, GL_VERTEX_SHADER);
    *frag = compile_glsl(frag_filename, GL_FRAGMENT_SHADER);
    if(!(*vert) || !(*frag)) return 0;
    *prog = glCreateProgram();
    glAttachShader(*prog, *vert);
    glAttachShader(*prog, *frag);
    glLinkProgram(*prog);
    /*  Print the info log if there were any errors */
    GLint link_status;
    glGetProgramiv(*prog, GL_LINK_STATUS, &link_status);
    if(!link_status) {
        GLint log_length;
        glGetProgramiv(*prog, GL_INFO_LOG_LENGTH, &log_length);
        GLchar log[log_length];
        glGetProgramInfoLog(*prog, log_length, &log_length, log);
        printf("Shader program info log:\n%s\n", log);
        return 0;
    }
    return 1;
}

int pg_shader_load(struct pg_shader* shader,
                   const char* vert_filename, const char* frag_filename)
{
    *shader = (struct pg_shader){ .mat_idx = { -1, -1, -1, -1, -1, -1, -1 } };
    return pg_compile_glsl(&shader->vert, &shader->frag, &shader->prog,
                           vert_filename, frag_filename);
}

/*  Code for managing created shaders   */
void pg_shader_link_matrix(struct pg_shader* shader, enum pg_matrix type,
                           const char* name)
{
    shader->mat_idx[type] = glGetUniformLocation(shader->prog, name);
}

void pg_shader_link_component(struct pg_shader* shader,
                              uint32_t comp, const char* name)
{
    if(!comp) return;
    int i = LEAST_SIGNIFICANT_BIT(comp);
    shader->component_idx[i] = glGetAttribLocation(shader->prog, name);
    shader->components |= comp;
}

void pg_shader_set_matrix(struct pg_shader* shader, enum pg_matrix type,
                          mat4 matrix)
{
    mat4_dup(shader->matrix[type], matrix);
    if(shader->mat_idx[type] != -1) {
        glUniformMatrix4fv(shader->mat_idx[type], 1, GL_FALSE, *matrix);
    }
}

void pg_shader_rebuild_matrices(struct pg_shader* shader)
{
    if(shader->mat_idx[PG_NORMAL_MATRIX] != -1) {
        mat4_invert(shader->matrix[PG_NORMAL_MATRIX],
                    shader->matrix[PG_MODEL_MATRIX]);
        glUniformMatrix4fv(shader->mat_idx[PG_NORMAL_MATRIX], 1, GL_TRUE,
                           *shader->matrix[PG_NORMAL_MATRIX]);
    }
    if(shader->mat_idx[PG_MODELVIEW_MATRIX] != -1) {
        mat4_mul(shader->matrix[PG_MODELVIEW_MATRIX],
                 shader->matrix[PG_VIEW_MATRIX],
                 shader->matrix[PG_MODEL_MATRIX]);
        glUniformMatrix4fv(shader->mat_idx[PG_MODELVIEW_MATRIX], 1, GL_FALSE,
                           *shader->matrix[PG_MODELVIEW_MATRIX]);
    }
    if(shader->mat_idx[PG_PROJECTIONVIEW_MATRIX] != -1) {
        mat4_mul(shader->matrix[PG_PROJECTIONVIEW_MATRIX],
                 shader->matrix[PG_PROJECTION_MATRIX],
                 shader->matrix[PG_VIEW_MATRIX]);
        glUniformMatrix4fv(shader->mat_idx[PG_PROJECTIONVIEW_MATRIX], 1, GL_FALSE,
                           *shader->matrix[PG_PROJECTIONVIEW_MATRIX]);
    }
    if(shader->mat_idx[PG_MVP_MATRIX] != -1) {
        mat4_mul(shader->matrix[PG_MVP_MATRIX],
                 shader->matrix[PG_MODEL_MATRIX],
                 shader->matrix[PG_VIEW_MATRIX]);
        mat4_mul(shader->matrix[PG_MVP_MATRIX],
                 shader->matrix[PG_PROJECTION_MATRIX],
                 shader->matrix[PG_MVP_MATRIX]);
        glUniformMatrix4fv(shader->mat_idx[PG_MVP_MATRIX], 1, GL_FALSE,
                           *shader->matrix[PG_MVP_MATRIX]);
    }
}

void pg_shader_buffer_model(struct pg_shader* shader, struct pg_model* model)
{
    if((shader->components & model->components) != shader->components) {
        printf("procgl shader error: Incompatible model!\n");
        return;
    }
    int i;
    struct pg_model_buffer* buf = NULL;
    struct pg_model_buffer* buf_iter;
    ARR_FOREACH_PTR(model->buffers, buf_iter, i) {
        if(buf_iter->shader->components == shader->components) {
            buf_iter->dirty_buffers = 0;
            buf = buf_iter;
        }
    }
    if(buf && buf->shader != shader) {
        /*  For shaders with identical requirements, share VBO and EBO  */
        struct pg_model_buffer new_buf = {
            .shader = shader, .dirty_buffers = 0, .vbo = buf->vbo };
        glGenVertexArrays(1, &new_buf.vao);
        ARR_PUSH(model->buffers, new_buf);
        buf = &model->buffers.data[model->buffers.len - 1];
    } else if(!buf) {
        /*  Initialize a new buffer for this shader-model pair  */
        struct pg_model_buffer new_buf;
        glGenBuffers(1, &new_buf.vbo);
        glGenVertexArrays(1, &new_buf.vao);
        new_buf.shader = shader;
        new_buf.dirty_buffers = 1;
        ARR_PUSH(model->buffers, new_buf);
        buf = &model->buffers.data[model->buffers.len - 1];
    }
    glBindVertexArray(buf->vao);
    glBindBuffer(GL_ARRAY_BUFFER, buf->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ebo);
    /*  Buffer the tris if they're dirty    */
    if(model->dirty_tris) {
        /*  We use the same index buffer for every one  */
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     model->tris.len * sizeof(*model->tris.data),
                     model->tris.data, GL_STATIC_DRAW);
    }
    model->dirty_tris = 0;
    size_t c_size[7] = {
        (shader->components & model->components & PG_MODEL_COMPONENT_POSITION) ?
            model->pos.len * sizeof(*model->pos.data) : 0,
        (shader->components & model->components & PG_MODEL_COMPONENT_COLOR) ?
            model->color.len * sizeof(*model->color.data) : 0,
        (shader->components & model->components & PG_MODEL_COMPONENT_UV) ?
            model->uv.len * sizeof(*model->uv.data) : 0,
        (shader->components & model->components & PG_MODEL_COMPONENT_NORMAL) ?
            model->normal.len * sizeof(*model->normal.data) : 0,
        (shader->components & model->components & PG_MODEL_COMPONENT_TAN_BITAN) ?
            model->tangent.len * sizeof(*model->tangent.data) : 0,
        (shader->components & model->components & PG_MODEL_COMPONENT_TAN_BITAN) ?
            model->bitangent.len * sizeof(*model->bitangent.data): 0,
        (shader->components & model->components & PG_MODEL_COMPONENT_HEIGHT) ?
            model->height.len * sizeof(*model->height.data) : 0 };
    size_t full_size = c_size[0] + c_size[1] + c_size[2] + c_size[3] +
                       c_size[4] + c_size[5] + c_size[6];
    glBufferData(GL_ARRAY_BUFFER, full_size, NULL, GL_STATIC_DRAW);
    size_t offset = 0;
    if(buf->dirty_buffers) {
        if(c_size[0]) {
            glBufferSubData(GL_ARRAY_BUFFER, 0, c_size[0], model->pos.data);
            offset += c_size[0];
        }
        if(c_size[1]) {
            glBufferSubData(GL_ARRAY_BUFFER, offset,
                            c_size[1], model->color.data);
            offset += c_size[1];
        }
        if(c_size[2]) {
            glBufferSubData(GL_ARRAY_BUFFER, offset,
                            c_size[2], model->uv.data);
            offset += c_size[2];
        }
        if(c_size[3]) {
            glBufferSubData(GL_ARRAY_BUFFER, offset,
                            c_size[3], model->normal.data);
            offset += c_size[3];
        }
        if(c_size[4]) {
            glBufferSubData(GL_ARRAY_BUFFER, offset,
                            c_size[4], model->tangent.data);
            offset += c_size[4];
        }
        if(c_size[5]) {
            glBufferSubData(GL_ARRAY_BUFFER, offset,
                            c_size[5], model->bitangent.data);
            offset += c_size[5];
        }
        if(c_size[6]) {
            glBufferSubData(GL_ARRAY_BUFFER, offset,
                            c_size[6], model->height.data);
            offset += c_size[6];
        }
    }
    offset = 0;
    if(c_size[0]) {
        glVertexAttribPointer(
            shader->component_idx[PG_MODEL_COMPONENT_POSITION_I],
            3, GL_FLOAT, GL_FALSE, 0, (void*)(offset));
        glEnableVertexAttribArray(shader->component_idx[PG_MODEL_COMPONENT_POSITION_I]);
        offset += c_size[0];
    }
    if(c_size[1]) {
        glVertexAttribPointer(
            shader->component_idx[PG_MODEL_COMPONENT_COLOR_I],
            4, GL_FLOAT, GL_FALSE, 0, (void*)(offset));
        glEnableVertexAttribArray(shader->component_idx[PG_MODEL_COMPONENT_COLOR_I]);
        offset += c_size[1];
    }
    if(c_size[2]) {
        glVertexAttribPointer(
            shader->component_idx[PG_MODEL_COMPONENT_UV_I],
            2, GL_FLOAT, GL_FALSE, 0, (void*)(offset));
        glEnableVertexAttribArray(shader->component_idx[PG_MODEL_COMPONENT_UV_I]);
        offset += c_size[2];
    }
    if(c_size[3]) {
        glVertexAttribPointer(
            shader->component_idx[PG_MODEL_COMPONENT_NORMAL_I],
            3, GL_FLOAT, GL_FALSE, 0, (void*)(offset));
        glEnableVertexAttribArray(shader->component_idx[PG_MODEL_COMPONENT_NORMAL_I]);
        offset += c_size[3];
    }
    if(c_size[4]) {
        glVertexAttribPointer(
            shader->component_idx[PG_MODEL_COMPONENT_TANGENT_I],
            3, GL_FLOAT, GL_FALSE, 0, (void*)(offset));
        glEnableVertexAttribArray(shader->component_idx[PG_MODEL_COMPONENT_TANGENT_I]);
        offset += c_size[4];
    }
    if(c_size[5]) {
        glVertexAttribPointer(
            shader->component_idx[PG_MODEL_COMPONENT_BITANGENT_I],
            3, GL_FLOAT, GL_FALSE, 0, (void*)(offset));
        glEnableVertexAttribArray(shader->component_idx[PG_MODEL_COMPONENT_BITANGENT_I]);
        offset += c_size[5];
    }
    if(c_size[6]) {
        glVertexAttribPointer(
            shader->component_idx[PG_MODEL_COMPONENT_HEIGHT_I],
            1, GL_FLOAT, GL_FALSE, 0, (void*)(offset));
        glEnableVertexAttribArray(shader->component_idx[PG_MODEL_COMPONENT_HEIGHT_I]);
        offset += c_size[6];
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    buf->dirty_buffers = 0;
}

int pg_shader_is_active(struct pg_shader* shader)
{
    return (shader == pg_active_shader);
}

void pg_shader_deinit(struct pg_shader* shader)
{
    glDeleteShader(shader->vert);
    glDeleteShader(shader->frag);
    glDeleteProgram(shader->prog);
    shader->deinit(shader->data);
}

void pg_shader_begin(struct pg_shader* shader, struct pg_viewer* view)
{
    pg_active_shader = shader;
    glUseProgram(shader->prog);
    shader->begin(shader, view);
}

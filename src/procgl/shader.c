#include <stdio.h>
#include <GL/glew.h>
#include "ext/linmath.h"
#include "viewer.h"
#include "shader.h"

/*  Shadow state for the currently used OpenGL shader   */
static struct pg_shader* pg_active_shader;

/*  Code for loading shaders dumped to headers  */
static GLuint compile_glsl_static(unsigned char* src, unsigned int len,
                                  GLenum type)
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
                           unsigned char* vert_src, unsigned int vert_len,
                           unsigned char* frag_src, unsigned int frag_len)
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
                          unsigned char* vert, unsigned int vert_len,
                          unsigned char* frag, unsigned int frag_len)
{
    *shader = (struct pg_shader){ .mat_idx = { -1, -1, -1, -1, -1, -1 } };
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
    *shader = (struct pg_shader){ .mat_idx = { -1, -1, -1, -1, -1, -1 } };
    return pg_compile_glsl(&shader->vert, &shader->frag, &shader->prog,
                           vert_filename, frag_filename);
}

/*  Code for managing created shaders   */
void pg_shader_link_matrix(struct pg_shader* shader, enum pg_matrix type,
                           const char* name)
{
    shader->mat_idx[type] = glGetUniformLocation(shader->prog, name);
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
    if(shader->mat_idx[PG_MODELVIEW_MATRIX] != -1) {
        mat4_mul(shader->matrix[PG_MODELVIEW_MATRIX],
                 shader->matrix[PG_MODEL_MATRIX],
                 shader->matrix[PG_VIEW_MATRIX]);
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

void pg_shader_buffer_attribs(struct pg_shader* shader)
{
    if(shader->buffer_attribs) shader->buffer_attribs(shader);
}

void pg_shader_begin(struct pg_shader* shader, struct pg_viewer* view)
{
    pg_active_shader = shader;
    glUseProgram(shader->prog);
    shader->begin(shader, view);
}

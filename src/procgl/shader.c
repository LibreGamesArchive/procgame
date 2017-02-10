#include <stdio.h>
#include <GL/glew.h>
#include "shader.h"

static GLuint load_shader(const char* filename, GLenum type)
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

int pg_shader_load(struct pg_shader* shader,
                   const char* vert_filename, const char* frag_filename)
{
    shader->vert = load_shader(vert_filename, GL_VERTEX_SHADER);
    shader->frag = load_shader(frag_filename, GL_FRAGMENT_SHADER);
    if(!(shader->vert) || !(shader->frag)) return 0;
    shader->prog = glCreateProgram();
    glAttachShader(shader->prog, shader->vert);
    glAttachShader(shader->prog, shader->frag);
    glLinkProgram(shader->prog);
    /*  Print the info log if there were any errors */
    GLint link_status;
    glGetProgramiv(shader->prog, GL_LINK_STATUS, &link_status);
    if(!link_status) {
        GLint log_length;
        glGetProgramiv(shader->prog, GL_INFO_LOG_LENGTH, &log_length);
        GLchar log[log_length];
        glGetProgramInfoLog(shader->prog, log_length, &log_length, log);
        printf("Shader program info log:\n%s\n", log);
        return 0;
    }
    return 1;
}

void pg_shader_deinit(struct pg_shader* shader)
{
    glDeleteShader(shader->vert);
    glDeleteShader(shader->frag);
    glDeleteProgram(shader->prog);
}

void pg_shader_buffer_attribs(struct pg_shader* shader)
{
    shader->buffer_attribs(shader);
}

void pg_shader_begin(struct pg_shader* shader)
{
    glUseProgram(shader->prog);
    shader->begin(shader);
}

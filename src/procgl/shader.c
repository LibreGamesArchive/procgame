#include "procgl.h"

/*  Shadow state for the currently used OpenGL shader   */
static struct pg_shader* pg_active_shader = NULL;

static inline void pg_shader_clear(struct pg_shader* shader)
{
    *shader = (struct pg_shader){};
    int i;
    for(i = 0; i < PG_COMMON_MATRICES; ++i) shader->matrix[i] = HTABLE_INVALID_ENTRY;
    for(i = 0; i < 8; ++i) shader->tex[i] = HTABLE_INVALID_ENTRY;
    for(i = 0; i < 8; ++i) shader->tex_rect[i] = HTABLE_INVALID_ENTRY;
    HTABLE_INIT(shader->uniforms, 8);
}

static void pg_shader_gather_uniforms(struct pg_shader* shader)
{
    GLint i, count, size;
    GLenum type; // type of the variable (float, vec3 or mat4, etc)
    const GLsizei bufSize = 32; // maximum name length
    GLchar name[bufSize]; // variable name in GLSL
    GLsizei length; // name length
    glGetProgramiv(shader->prog, GL_ACTIVE_UNIFORMS, &count);
    printf("Active Uniforms: %d\n", count);
    for (i = 0; i < count; i++)
    {
        glGetActiveUniform(shader->prog, (GLuint)i, bufSize, &length, &size, &type, name);
        GLint location = glGetUniformLocation(shader->prog, name);
        if(name[0] == '_') continue;
        if(strncmp(name, "pg_fbtexture_", sizeof("pg_fbtexture_") - 1) == 0) {
            int tex_id = name[sizeof("pg_fbtexture_") - 1] - '0';
            if(tex_id < 0 || tex_id >= 8) {
                char* name_offset = name + sizeof("pg_fbtexture_") - 1;
                if(strncmp(name_offset, "depth", 8) == 0) {
                    printf("Caught pg_fbtexture_depth: %d\n", i);
                    HTABLE_SET(shader->uniforms, name, (struct pg_shader_uniform) {
                        .type = PG_TEXTURE, .array_len = 1, .idx = location});
                    HTABLE_GET_ENTRY(shader->uniforms, name, shader->fbdepth);
                } else {
                    printf("Shader error: bad pg_fbtexture: %s\n", name_offset);
                }
                continue;
            }
            printf("Caught pg_fbtexture_%d: %d\n", tex_id, i);
            HTABLE_SET(shader->uniforms, name, (struct pg_shader_uniform) {
                .type = PG_TEXTURE, .array_len = 1, .idx = location});
            HTABLE_GET_ENTRY(shader->uniforms, name, shader->fbtex[tex_id]);
        } else if(strncmp(name, "pg_texture_", sizeof("pg_texture_") - 1) == 0) {
            int tex_id = name[sizeof("pg_texture_") - 1] - '0';
            if(tex_id < 0 || tex_id >= 8) {
                printf("Shader error: bad pg_texture id: %d\n", tex_id);
                continue;
            }
            printf("Caught pg_texture_%d: %d\n", tex_id, i);
            HTABLE_SET(shader->uniforms, name, (struct pg_shader_uniform) {
                .type = PG_TEXTURE, .array_len = 1, .idx = location});
            HTABLE_GET_ENTRY(shader->uniforms, name, shader->tex[tex_id]);
        } else if(strncmp(name, "pg_tex_rect_", sizeof("pg_tex_rect_") - 1) == 0) {
            int tex_id = name[sizeof("pg_tex_rect_") - 1] - '0';
            if(tex_id < 0 || tex_id >= 8) {
                printf("Shader error: bad pg_tex_rect id: %d\n", tex_id);
                continue;
            }
            printf("Caught pg_tex_rect_%d\n", tex_id);
            HTABLE_SET(shader->uniforms, name, (struct pg_shader_uniform) {
                .type = PG_VEC4, .array_len = 1, .idx = location});
            HTABLE_GET_ENTRY(shader->uniforms, name, shader->tex_rect[tex_id]);
        } else if(strncmp(name, "pg_matrix_", sizeof("pg_matrix_") - 1) == 0) {
            static const char* mat_names[] = {
                [PG_MODEL_MATRIX] = "model",
                [PG_NORMAL_MATRIX] = "normal",
                [PG_VIEW_MATRIX] = "view",
                [PG_PROJECTION_MATRIX] = "projection",
                [PG_MODELVIEW_MATRIX] = "modelview",
                [PG_PROJECTIONVIEW_MATRIX] = "projview",
                [PG_MVP_MATRIX] = "mvp" };
            int j;
            for(j = 0; j < PG_COMMON_MATRICES; ++j) {
                char* name_offset = name + sizeof("pg_matrix_") - 1;
                int max_len = 32 - sizeof("pg_matrix_") - 1;
                if(strncmp(name_offset, mat_names[j], max_len) == 0) {
                    printf("Caught pg_matrix_%s\n", mat_names[j]);
                    HTABLE_SET(shader->uniforms, name, (struct pg_shader_uniform) {
                        .type = PG_MATRIX, .array_len = 1, .idx = location});
                    HTABLE_GET_ENTRY(shader->uniforms, name, shader->matrix[j]);
                    break;
                }
            }
        } else {
            enum pg_data_type pg_type = PG_NULL;
            switch(type) {
                case GL_INT: pg_type = PG_INT; break;
                case GL_INT_VEC2: pg_type = PG_IVEC2; break;
                case GL_INT_VEC3: pg_type = PG_IVEC3; break;
                case GL_INT_VEC4: pg_type = PG_IVEC4; break;
                case GL_UNSIGNED_INT: pg_type = PG_UINT; break;
                case GL_UNSIGNED_INT_VEC2: pg_type = PG_UVEC2; break;
                case GL_UNSIGNED_INT_VEC3: pg_type = PG_UVEC3; break;
                case GL_UNSIGNED_INT_VEC4: pg_type = PG_UVEC4; break;
                case GL_FLOAT: pg_type = PG_FLOAT; break;
                case GL_FLOAT_VEC2: pg_type = PG_VEC2; break;
                case GL_FLOAT_VEC3: pg_type = PG_VEC3; break;
                case GL_FLOAT_VEC4: pg_type = PG_VEC4; break;
                case GL_FLOAT_MAT4: pg_type = PG_MATRIX; break;
                case GL_SAMPLER_2D: pg_type = PG_TEXTURE; break;
                default: break;
            }
            if(pg_type) {
                if(size > 1) {
                    char* arr_def = strchr(name, '[');
                    if(arr_def) arr_def[0] = '\0';
                    printf("    Array length %d\n", size);
                }
                HTABLE_SET(shader->uniforms, name,
                    (struct pg_shader_uniform){
                        .type = pg_type, .idx = location, .array_len = size });
                printf("Found custom uniform %s\n", name);
            } else {
                printf("Unrecognized uniform type for %s\n", name);
            }
        }
    }
}

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
    pg_shader_clear(shader);
    return pg_compile_glsl_static(&shader->vert, &shader->frag, &shader->prog,
                                  vert, vert_len, frag, frag_len);
}

/*  Code for loading shaders from files dynamically */
static GLuint compile_glsl(const char* filename, GLenum type)
{
    /*  Read the file into a buffer */
    FILE* f = fopen(filename, "r");
    if(!f) {
        printf("Could not read shader source file: %s\n", filename);
        return 0;
    }
    int len;
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    GLchar source[len+1];
    const GLchar* double_ptr = source; /*  GL requires this   */
    fseek(f, 0, SEEK_SET);
    int r = fread(source, 1, len, f);
    source[r] = '\0';
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

static void shader_bind_attribs(GLuint prog)
{
    glBindAttribLocation(prog, PG_ATTRIB_POSITION, "v_position");
    glBindAttribLocation(prog, PG_ATTRIB_COLOR, "v_color");
    glBindAttribLocation(prog, PG_ATTRIB_TEXCOORD, "v_tex_coord");
    glBindAttribLocation(prog, PG_ATTRIB_NORMAL, "v_normal");
    glBindAttribLocation(prog, PG_ATTRIB_TANGENT, "v_tangent");
    glBindAttribLocation(prog, PG_ATTRIB_BITANGENT, "v_bitangent");
    glBindAttribLocation(prog, PG_ATTRIB_HEIGHT, "v_height");
}

int pg_compile_glsl(GLuint* vert, GLuint* frag, GLuint* prog,
                   const char* vert_filename, const char* frag_filename)
{
    *vert = compile_glsl(vert_filename, GL_VERTEX_SHADER);
    *frag = compile_glsl(frag_filename, GL_FRAGMENT_SHADER);
    if(!(*vert) || !(*frag)) return 0;
    *prog = glCreateProgram();
    shader_bind_attribs(*prog);
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
    pg_shader_clear(shader);
    int ret = pg_compile_glsl(&shader->vert, &shader->frag, &shader->prog,
                           vert_filename, frag_filename);
    if(ret == 1) {
        pg_shader_gather_uniforms(shader);
    }
    return ret;
}

void pg_shader_set_matrix(struct pg_shader* shader, enum pg_matrix type,
                          mat4 matrix)
{
    struct pg_shader_uniform* dst;
    HTABLE_ENTRY_PTR(shader->uniforms, shader->matrix[type], dst);
    if(!dst) return;
    mat4_dup(dst->data.m, matrix);
    if(pg_active_shader == shader) {
        glUniformMatrix4fv(dst->idx, 1, GL_FALSE, *matrix);
    }
}

void pg_shader_set_texture(struct pg_shader* shader,
                           int idx, struct pg_texture* tex)
{
    if(idx < 0 || idx >= 8) return;
    struct pg_shader_uniform* dst;
    HTABLE_ENTRY_PTR(shader->uniforms, shader->tex[idx], dst);
    if(!dst) return;
    dst->data.tex = tex;
    if(pg_active_shader == shader) {
        glActiveTexture(GL_TEXTURE0 + idx);
        glBindTexture(GL_TEXTURE_2D, tex->handle);
        glUniform1i(dst->idx, idx);
    }
}

void pg_shader_set_tex_rect(struct pg_shader* shader,
                            int idx, vec4 tex_rect)
{
    if(idx < 0 || idx >= 8) return;
    struct pg_shader_uniform* dst;
    HTABLE_ENTRY_PTR(shader->uniforms, shader->tex[idx], dst);
    if(!dst) return;
    vec4_dup(dst->data.f, tex_rect);
    if(pg_active_shader == shader) {
        glUniform4f(dst->idx, tex_rect[0], tex_rect[1], tex_rect[2], tex_rect[4]);
    }
}

static void set_uniform(struct pg_shader_uniform* dst, struct pg_uniform* src)
{
    if(!dst || dst->idx == -1) return;
    switch(dst->type) {
        case PG_INT: glUniform1i(dst->idx, src->i[0]); break;
        case PG_IVEC2: glUniform2i(dst->idx, src->i[0], src->i[1]); break;
        case PG_IVEC3: glUniform3i(dst->idx, src->i[0], src->i[1], src->i[2]); break;
        case PG_IVEC4: glUniform4i(dst->idx, src->i[0], src->i[1], src->i[2], src->i[3]); break;
        case PG_UINT: glUniform1i(dst->idx, src->u[0]); break;
        case PG_UVEC2: glUniform2ui(dst->idx, src->u[0], src->u[1]); break;
        case PG_UVEC3: glUniform3ui(dst->idx, src->u[0], src->u[1], src->u[2]); break;
        case PG_UVEC4: glUniform4ui(dst->idx, src->u[0], src->u[1], src->u[2], src->u[3]); break;
        case PG_FLOAT: glUniform1f(dst->idx, src->f[0]); break;
        case PG_VEC2: glUniform2f(dst->idx, src->f[0], src->f[1]); break;
        case PG_VEC3: glUniform3f(dst->idx, src->f[0], src->f[1], src->f[2]); break;
        case PG_VEC4: glUniform4f(dst->idx, src->f[0], src->f[1], src->f[2], src->f[3]); break;
        case PG_MATRIX: glUniformMatrix4fv(dst->idx, 1, GL_FALSE, *src->m); break;
        case PG_TEXTURE: glUniform1i(dst->idx, src->tex->handle); break;
        default: return;
    }
    dst->data = *src;
}

void pg_shader_uniform_by_name(struct pg_shader* shader,
                               char* name, struct pg_uniform* uni)
{
    struct pg_shader_uniform* sh_uni;
    HTABLE_GET(shader->uniforms, name, sh_uni);
    set_uniform(sh_uni, uni);
}

pg_shader_uniform_t pg_shader_get_uniform(struct pg_shader* shader, char* name)
{
    pg_shader_uniform_t uni;
    HTABLE_GET_ENTRY(shader->uniforms, name, uni);
    return uni;
}

void pg_shader_uniforms_from_table(struct pg_shader* shader,
                                   pg_shader_uniform_table_t* table)
{
    HTABLE_ITER sh_iter;
    struct pg_shader_uniform* sh_uni;
    struct pg_shader_uniform* t_uni;
    char* key;
    HTABLE_ITER_BEGIN(shader->uniforms, sh_iter);
    while(!HTABLE_ITER_END(shader->uniforms, sh_iter)) {
        HTABLE_ITER_NEXT(shader->uniforms, sh_iter, key, sh_uni);
        if(sh_uni->idx == -1) continue;
        HTABLE_GET(*table, key, t_uni);
        if(!t_uni || sh_uni->type != t_uni->type) continue;
        set_uniform(sh_uni, &t_uni->data);
    }
}

void pg_shader_uniform(struct pg_shader* shader, pg_shader_uniform_t idx,
                       struct pg_uniform* uni)
{
    struct pg_shader_uniform* sh_uni = NULL;
    HTABLE_ENTRY_PTR(shader->uniforms, idx, sh_uni);
    if(!sh_uni) return;
    set_uniform(sh_uni, uni);
}

void pg_shader_deinit(struct pg_shader* shader)
{
    glDeleteShader(shader->vert);
    glDeleteShader(shader->frag);
    glDeleteProgram(shader->prog);
    HTABLE_DEINIT(shader->uniforms);
}

void pg_shader_begin(struct pg_shader* shader)
{
    pg_active_shader = shader;
    glUseProgram(shader->prog);
}

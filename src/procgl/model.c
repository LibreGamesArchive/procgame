#include <GL/glew.h>
#include "vertex.h"
#include "shader.h"
#include "texture.h"
#include "model.h"

void pg_model_init(struct pg_model* model)
{
    ARR_INIT(model->verts);
    ARR_INIT(model->tris);
    glGenBuffers(1, &model->verts_gl);
    glGenBuffers(1, &model->tris_gl);
    glGenVertexArrays(1, &model->vao);
}

void pg_model_deinit(struct pg_model* model)
{
    ARR_DEINIT(model->verts);
    ARR_DEINIT(model->tris);
    glDeleteBuffers(1, &model->verts_gl);
    glDeleteBuffers(1, &model->tris_gl);
    glDeleteVertexArrays(1, &model->vao);
}

void pg_model_buffer(struct pg_model* model, struct pg_shader* shader)
{
    glBindVertexArray(model->vao);
    glBindBuffer(GL_ARRAY_BUFFER, model->verts_gl);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->tris_gl);
    glBufferData(GL_ARRAY_BUFFER, model->verts.len * sizeof(struct pg_vert3d),
                 model->verts.data, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->tris.len * sizeof(unsigned),
                 model->tris.data, GL_STATIC_DRAW);
    pg_shader_buffer_attribs(shader);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void pg_model_begin(struct pg_model* model)
{
    glBindVertexArray(model->vao);
}

void pg_model_draw(struct pg_model* model, struct pg_shader* shader,
                   mat4 transform)
{
    pg_shader_set_matrix(shader, PG_MODEL_MATRIX, transform);
    pg_shader_rebuild_matrices(shader);
    glDrawElements(GL_TRIANGLES, model->tris.len, GL_UNSIGNED_INT, 0);
}

void pg_model_split_tris(struct pg_model* model)
{
    struct pg_vert3d* v[3];
    unsigned t[3];
    struct pg_vert3d new_v[model->tris.len];
    int i;
    for(i = 0; i < model->tris.len; i += 3) {
        t[0] = model->tris.data[i];
        t[1] = model->tris.data[i + 1];
        t[2] = model->tris.data[i + 2];
        v[0] = &model->verts.data[t[0]];
        v[1] = &model->verts.data[t[1]];
        v[2] = &model->verts.data[t[2]];
        new_v[i] = *v[0];
        new_v[i + 1] = *v[1];
        new_v[i + 2] = *v[2];
        model->tris.data[i] = i;
        model->tris.data[i + 1] = i + 1;
        model->tris.data[i + 2] = i + 2;
    }
    pg_model_reserve_verts(model, model->tris.len);
    memcpy(model->verts.data, new_v, sizeof(new_v));
}

void pg_model_precalc_verts(struct pg_model* model)
{
    struct pg_vert3d* v[3];
    unsigned int t[3];
    int i;
    for(i = 0; i < model->tris.len; i += 3) {
        t[0] = model->tris.data[i];
        t[1] = model->tris.data[i + 1];
        t[2] = model->tris.data[i + 2];
        v[0] = &model->verts.data[t[0]];
        v[1] = &model->verts.data[t[1]];
        v[2] = &model->verts.data[t[2]];
        /*  First we calculate the normal for each vertex   */
        vec3 norm;
        vec3 edge0, edge1;
        vec3_sub(edge0, v[0]->pos, v[1]->pos);
        vec3_sub(edge1, v[0]->pos, v[2]->pos);
        vec3_mul_cross(norm, edge0, edge1);
        vec3_normalize(norm, norm);
        /*  Next we calculate the tangent and bitangent in alignment with the
            tex_coords  */
        vec2 tex_d0, tex_d1;
        vec2_sub(tex_d0, v[1]->tex_coord, v[0]->tex_coord);
        vec2_sub(tex_d1, v[2]->tex_coord, v[0]->tex_coord);
        float r = 1.0f / (tex_d0[0] * tex_d1[1] - tex_d0[1] * tex_d1[0]);
        vec3 tmp0, tmp1, tangent, bitangent;
        vec3_scale(tmp0, edge0, tex_d1[1]);
        vec3_scale(tmp1, edge1, tex_d0[1]);
        vec3_sub(tangent, tmp0, tmp1);
        vec3_scale(tangent, tangent, r);
        vec3_scale(tmp0, edge1, tex_d0[0]);
        vec3_scale(tmp1, edge0, tex_d1[0]);
        vec3_sub(bitangent, tmp0, tmp1);
        vec3_scale(bitangent, bitangent, r);
        /*  Now we average the calculated values with any existing values,
            so each vertex will have an average value for each associated
            face    */
        int j;
        for(j = 0; j < 3; ++j) {
            vec3_add(v[j]->normal, norm, v[j]->normal);
            vec3_normalize(v[j]->normal, v[j]->normal);
            vec3_add(v[j]->tangent, tangent, v[j]->tangent);
            vec3_normalize(v[j]->tangent, v[j]->tangent);
            vec3_add(v[j]->bitangent, bitangent, v[j]->bitangent);
            vec3_normalize(v[j]->bitangent, v[j]->bitangent);
        }
    }
}

void pg_model_generate_texture(struct pg_model* model,
                                   struct pg_texture* texture,
                                   unsigned w, unsigned h)
{
    struct pg_vert3d* v[3];
    unsigned t[3];
    int i;
    for(i = 0; i < model->tris.len; i += 3) {
        t[0] = model->tris.data[i];
        t[1] = model->tris.data[i + 1];
        t[2] = model->tris.data[i + 2];
        v[0] = &model->verts.data[t[0]];
        v[1] = &model->verts.data[t[1]];
        v[2] = &model->verts.data[t[2]];
        vec3 norm;
        vec3 edge0, edge1;
        vec3 b_x, b_y;
        vec3_sub(edge0, v[0]->pos, v[1]->pos);
        vec3_sub(edge1, v[0]->pos, v[2]->pos);
        vec3_normalize(edge0, edge0);
        vec3_normalize(edge1, edge1);
        vec3_mul_cross(norm, edge0, edge1);
        vec3_normalize(norm, norm);
        vec3_dup(b_x, edge0);
        vec3_mul_cross(b_y, edge0, norm);
        int j;
        vec2 tex[3];
        for(j = 0; j < 3; ++j) {
            tex[j][0] = vec3_mul_inner(v[j]->pos, b_x);
            tex[j][1] = vec3_mul_inner(v[j]->pos, b_y);
        }
    }
}

void pg_model_reserve_verts(struct pg_model* model, unsigned count)
{
    ARR_RESERVE(model->verts, count);
    model->verts.len = count;
}

void pg_model_reserve_tris(struct pg_model* model, unsigned count)
{
    ARR_RESERVE(model->tris, count * 3);
    model->tris.len = count * 3;
}

unsigned pg_model_add_vertex(struct pg_model* model, struct pg_vert3d* vert)
{
    ARR_PUSH(model->verts, *vert);
    return model->verts.len - 1;
}

void pg_model_add_triangle(struct pg_model* model, unsigned v0,
                               unsigned v1, unsigned v2)
{
    int i = model->tris.len;
    ARR_RESERVE(model->tris, i + 3);
    model->tris.len += 3;
    model->tris.data[i] = v0;
    model->tris.data[i + 1] = v1;
    model->tris.data[i + 2] = v2;
}

void pg_model_append(struct pg_model* dst, struct pg_model* src,
                         mat4 transform)
{
    int dst_vert_len = dst->verts.len;
    int dst_tri_len = dst->tris.len;
    int i;
    struct pg_vert3d* v;
    ARR_RESERVE(dst->verts, dst_vert_len + src->verts.len);
    ARR_FOREACH_PTR(src->verts, v, i) {
        vec4 old = { v->pos[0], v->pos[1], v->pos[2], 1.0f };
        vec4 new;
        mat4_mul_vec4(new, transform, old);
        mat4 inv;
        mat4_invert(inv, transform);
        mat4 norm;
        mat4_transpose(norm, inv);
        vec4 old_norm = { v->normal[0], v->normal[1], v->normal[2], 0.0f };
        vec4 new_norm;
        mat4_mul_vec4(new_norm, norm, old_norm);
        dst->verts.data[dst_vert_len + i] =
            (struct pg_vert3d){ .pos = { new[0], new[1], new[2] },
                             .normal = { new_norm[0], new_norm[1], new_norm[2] },
                             .tex_coord = { v->tex_coord[0], v->tex_coord[1] } };
    }
    dst->verts.len += src->verts.len;
    ARR_RESERVE(dst->tris, dst_tri_len + src->tris.len);
    unsigned t;
    ARR_FOREACH(src->tris, t, i) {
        dst->tris.data[dst->tris.len + i] = t + dst_tri_len;
    }
    dst->tris.len += src->tris.len;
}

void pg_model_transform(struct pg_model* model, mat4 transform)
{
    int i;
    struct pg_vert3d* v;
    ARR_FOREACH_PTR(model->verts, v, i) {
        vec4 old = { v->pos[0], v->pos[1], v->pos[2], 1.0f };
        vec4 new;
        mat4_mul_vec4(new, transform, old);
        mat4 inv;
        mat4_invert(inv, transform);
        mat4 norm;
        mat4_transpose(norm, inv);
        vec4 old_norm = { v->normal[0], v->normal[1], v->normal[2], 0.0f };
        vec4 new_norm;
        mat4_mul_vec4(new_norm, norm, old_norm);
        *v = (struct pg_vert3d){ .pos = { new[0], new[1], new[2] },
                              .normal = { new_norm[0], new_norm[1], new_norm[2] },
                              .tex_coord = { v->tex_coord[0], v->tex_coord[1] } };
    }
}


#include <stdio.h>
#include <GL/glew.h>
#include "ext/linmath.h"
#include "arr.h"
#include "viewer.h"
#include "model.h"
#include "shader.h"
#include "wave.h"
#include "heightmap.h"
#include "texture.h"

static void pg_model_reset_buffers(struct pg_model* model);

/*  Setup+cleanup   */
void pg_model_init(struct pg_model* model)
{
    model->components = 0;
    model->v_count = 0;
    model->active = -1;
    model->dirty_tris = 1;
    model->ebo = 0;
    glGenBuffers(1, &model->ebo);
    ARR_INIT(model->pos);
    ARR_INIT(model->color);
    ARR_INIT(model->uv);
    ARR_INIT(model->normal);
    ARR_INIT(model->tangent);
    ARR_INIT(model->bitangent);
    ARR_INIT(model->height);
    ARR_INIT(model->buffers);
    ARR_INIT(model->tris);
}

void pg_model_reset(struct pg_model* model)
{
    ARR_TRUNCATE(model->pos, 0);
    ARR_TRUNCATE(model->color, 0);
    ARR_TRUNCATE(model->uv, 0);
    ARR_TRUNCATE(model->normal, 0);
    ARR_TRUNCATE(model->tangent, 0);
    ARR_TRUNCATE(model->bitangent, 0);
    ARR_TRUNCATE(model->height, 0);
    pg_model_reset_buffers(model);
    ARR_TRUNCATE(model->tris, 0);
    model->components = 0;
}

void pg_model_deinit(struct pg_model* model)
{
    ARR_DEINIT(model->pos);
    ARR_DEINIT(model->color);
    ARR_DEINIT(model->uv);
    ARR_DEINIT(model->normal);
    ARR_DEINIT(model->tangent);
    ARR_DEINIT(model->bitangent);
    ARR_DEINIT(model->height);
    pg_model_reset_buffers(model);
    ARR_DEINIT(model->buffers);
    ARR_DEINIT(model->tris);
    glDeleteBuffers(1, &model->ebo);
}

/*  Shader handling */
void pg_model_buffer(struct pg_model* model)
{
    glBindVertexArray(0);
    if(model->dirty_tris) {
        /*  We use the same index buffer for every one  */
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     model->tris.len * sizeof(*model->tris.data),
                     model->tris.data, GL_STATIC_DRAW);
    }
    model->dirty_tris = 0;
    struct pg_model_buffer* buf;
    int i;
    ARR_FOREACH_PTR(model->buffers, buf, i) buf->dirty_buffers = 1;
    ARR_FOREACH_PTR(model->buffers, buf, i) {
        if(buf->dirty_buffers) {
            pg_shader_buffer_model(buf->shader, model);
        }
    }
}

void pg_model_begin(struct pg_model* model, struct pg_shader* shader)
{
    struct pg_model_buffer* m_i;
    int i;
    ARR_FOREACH_PTR(model->buffers, m_i, i) {
        if(m_i->shader == shader) break;
    }
    if(m_i->shader != shader) {
        printf("procgl render error: model has not been buffered for shader\n");
        return;
    }
    glBindVertexArray(m_i->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ebo);
    model->active = i;
}

void pg_model_draw(struct pg_model* model, mat4 transform)
{
    if(model->active < 0) return;
    struct pg_shader* m_shader = model->buffers.data[model->active].shader;
    pg_shader_set_matrix(m_shader, PG_MODEL_MATRIX, transform);
    pg_shader_rebuild_matrices(m_shader);
    glDrawElements(GL_TRIANGLES, model->tris.len, GL_UNSIGNED_INT, 0);
}

/*  Raw vertex/triangle building    */
void pg_model_reserve_verts(struct pg_model* model, unsigned count)
{
    model->v_count = count;
    if(model->components & PG_MODEL_COMPONENT_POSITION) {
        ARR_RESERVE(model->pos, count);
        ARR_TRUNCATE_CLEAR(model->pos, model->pos.len);
        model->pos.len = count;
    }
    if(model->components & PG_MODEL_COMPONENT_COLOR) {
        ARR_RESERVE(model->color, count);
        ARR_TRUNCATE_CLEAR(model->color, model->color.len);
        model->color.len = count;
    }
    if(model->components & PG_MODEL_COMPONENT_UV) {
        ARR_RESERVE(model->uv, count);
        ARR_TRUNCATE_CLEAR(model->uv, model->uv.len);
        model->uv.len = count;
    }
    if(model->components & PG_MODEL_COMPONENT_NORMAL) {
        ARR_RESERVE(model->normal, count);
        ARR_TRUNCATE_CLEAR(model->normal, model->normal.len);
        model->normal.len = count;
    }
    if(model->components & PG_MODEL_COMPONENT_TANGENT) {
        ARR_RESERVE(model->tangent, count);
        ARR_TRUNCATE_CLEAR(model->tangent, model->tangent.len);
        model->tangent.len = count;
    }
    if(model->components & PG_MODEL_COMPONENT_BITANGENT) {
        ARR_RESERVE(model->bitangent, count);
        ARR_TRUNCATE_CLEAR(model->bitangent, model->bitangent.len);
        model->bitangent.len = count;
    }
    if(model->components & PG_MODEL_COMPONENT_HEIGHT) {
        ARR_RESERVE(model->height, count);
        ARR_TRUNCATE_CLEAR(model->height, model->height.len);
        model->height.len = count;
    }
}

void pg_model_reserve_tris(struct pg_model* model, unsigned count)
{
    ARR_RESERVE(model->tris, count * 3);
    model->tris.len = count * 3;
}

void pg_model_set_vertex(struct pg_model* model, struct pg_vertex_full* v,
                         unsigned i)
{
    if(i >= model->v_count) return;
    if(v->components & model->components & PG_MODEL_COMPONENT_POSITION) {
        vec3_dup(model->pos.data[i].v, v->pos);
    }
    if(v->components & model->components & PG_MODEL_COMPONENT_COLOR) {
        vec4_dup(model->color.data[i].v, v->color);
    }
    if(v->components & model->components & PG_MODEL_COMPONENT_UV) {
        vec2_dup(model->uv.data[i].v, v->uv);
    }
    if(v->components & model->components & PG_MODEL_COMPONENT_NORMAL) {
        vec3_dup(model->normal.data[i].v, v->normal);
    }
    if(v->components & model->components & PG_MODEL_COMPONENT_TANGENT) {
        vec3_dup(model->tangent.data[i].v, v->tangent);
    }
    if(v->components & model->components & PG_MODEL_COMPONENT_BITANGENT) {
        vec3_dup(model->bitangent.data[i].v, v->bitangent);
    }
    if(v->components && model->components & PG_MODEL_COMPONENT_HEIGHT) {
        model->height.data[i] = v->height;
    }
}

unsigned pg_model_add_vertex(struct pg_model* model, struct pg_vertex_full* v)
{
    if(v->components & model->components & PG_MODEL_COMPONENT_POSITION) {
        vec3_t vs = {{ v->pos[0], v->pos[1], v->pos[2] }};
        ARR_PUSH(model->pos, vs);
    }
    if(v->components & model->components & PG_MODEL_COMPONENT_COLOR) {
        vec4_t vs = {{ v->color[0], v->color[1], v->color[2], v->color[3] }};
        ARR_PUSH(model->color, vs);
    }
    if(v->components & model->components & PG_MODEL_COMPONENT_UV) {
        vec2_t vs = {{ v->uv[0], v->uv[1] }};
        ARR_PUSH(model->uv, vs);
    }
    if(v->components & model->components & PG_MODEL_COMPONENT_NORMAL) {
        vec3_t vs = {{ v->normal[0], v->normal[1], v->normal[2] }};
        ARR_PUSH(model->normal, vs);
    }
    if(v->components & model->components & PG_MODEL_COMPONENT_TANGENT) {
        vec3_t vs = {{ v->tangent[0], v->tangent[1], v->tangent[2] }};
        ARR_PUSH(model->tangent, vs);
    }
    if(v->components & model->components & PG_MODEL_COMPONENT_BITANGENT) {
        vec3_t vs = {{ v->bitangent[0], v->bitangent[1], v->bitangent[2] }};
        ARR_PUSH(model->bitangent, vs);
    }
    if(v->components & model->components & PG_MODEL_COMPONENT_HEIGHT) {
        ARR_PUSH(model->height, v->height);
    }
    return model->v_count++;
}

void pg_model_get_vertex(struct pg_model* model, struct pg_vertex_full* out,
                         unsigned i)
{
    if(i >= model->v_count) return;
    *out = (struct pg_vertex_full){ .components = model->components };
    if(model->components & PG_MODEL_COMPONENT_POSITION) {
        vec3_dup(out->pos, model->pos.data[i].v);
    }
    if(model->components & PG_MODEL_COMPONENT_COLOR) {
        vec4_dup(out->color, model->color.data[i].v);
    }
    if(model->components & PG_MODEL_COMPONENT_UV) {
        vec2_dup(out->uv, model->uv.data[i].v);
    }
    if(model->components & PG_MODEL_COMPONENT_NORMAL) {
        vec3_dup(out->normal, model->normal.data[i].v);
    }
    if(model->components & PG_MODEL_COMPONENT_TANGENT) {
        vec3_dup(out->tangent, model->tangent.data[i].v);
    }
    if(model->components & PG_MODEL_COMPONENT_BITANGENT) {
        vec3_dup(out->bitangent, model->bitangent.data[i].v);
    }
    if(model->components & PG_MODEL_COMPONENT_HEIGHT) {
        out->height = model->height.data[i];
    }
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

/*  Compos/transformation  */
void pg_model_append(struct pg_model* dst, struct pg_model* src,
                         mat4 transform)
{
    unsigned dst_v = dst->v_count;
    struct pg_vertex_full v;
    int src_i;
    for(src_i = 0; src_i < src->v_count; ++src_i) {
        pg_model_get_vertex(src, &v, src_i);
        vec4 old = { v.pos[0], v.pos[1], v.pos[2], 1.0f };
        vec4 new;
        mat4_mul_vec4(new, transform, old);
        mat4 inv;
        mat4_invert(inv, transform);
        mat4 norm;
        mat4_transpose(norm, inv);
        vec4 old_norm = { v.normal[0], v.normal[1], v.normal[2], 0.0f };
        vec4 old_tan = { v.tangent[0], v.tangent[1], v.tangent[2], 0.0f };
        vec4 old_bitan = { v.bitangent[0], v.bitangent[1], v.bitangent[2], 0.0f };
        vec4 new_norm, new_tan, new_bitan;
        mat4_mul_vec4(new_norm, norm, old_norm);
        mat4_mul_vec4(new_tan, norm, old_tan);
        mat4_mul_vec4(new_bitan, norm, old_bitan);
        vec4_normalize(new_norm, new_norm);
        vec4_normalize(new_tan, new_tan);
        vec4_normalize(new_bitan, new_bitan);
        v = (struct pg_vertex_full) {
            .pos = { new[0], new[1], new[2] },
            .color = { v.color[0], v.color[1], v.color[2], v.color[3] },
            .uv = { v.uv[0], v.uv[1] },
            .normal = { new_norm[0], new_norm[1], new_norm[2] },
            .tangent = { new_tan[0], new_tan[1], new_tan[2] },
            .bitangent = { new_bitan[0], new_bitan[1], new_bitan[2] },
            .height = v.height };
        pg_model_add_vertex(dst, &v);
    }
    ARR_RESERVE(dst->tris, dst->tris.len + src->tris.len);
    unsigned t;
    int i;
    ARR_FOREACH(src->tris, t, i) {
        dst->tris.data[dst->tris.len + i] = t + dst_v;
    }
    dst->tris.len += src->tris.len;
}

void pg_model_transform(struct pg_model* model, mat4 transform)
{
    int i;
    struct pg_vertex_full v;
    for(i = 0; i < model->v_count; ++i) {
        pg_model_get_vertex(model, &v, i);
        vec4 old = { v.pos[0], v.pos[1], v.pos[2], 1.0f };
        vec4 new;
        mat4_mul_vec4(new, transform, old);
        mat4 inv;
        mat4_invert(inv, transform);
        mat4 norm;
        mat4_transpose(norm, inv);
        vec4 old_norm = { v.normal[0], v.normal[1], v.normal[2], 0.0f };
        vec4 old_tan = { v.tangent[0], v.tangent[1], v.tangent[2], 0.0f };
        vec4 old_bitan = { v.bitangent[0], v.bitangent[1], v.bitangent[2], 0.0f };
        vec4 new_norm, new_tan, new_bitan;
        mat4_mul_vec4(new_norm, norm, old_norm);
        mat4_mul_vec4(new_tan, norm, old_tan);
        mat4_mul_vec4(new_bitan, norm, old_bitan);
        vec4_normalize(new_norm, new_norm);
        vec4_normalize(new_tan, new_tan);
        vec4_normalize(new_bitan, new_bitan);
        v = (struct pg_vertex_full) { model->components,
            .pos = { new[0], new[1], new[2] },
            .color = { v.color[0], v.color[1], v.color[2], v.color[3] },
            .uv = { v.uv[0], v.uv[1] },
            .normal = { new_norm[0], new_norm[1], new_norm[2] },
            .tangent = { new_tan[0], new_tan[1], new_tan[2] },
            .bitangent = { new_bitan[0], new_bitan[1], new_bitan[2] },
            .height = v.height };
        pg_model_set_vertex(model, &v, i);
    }
}

/*  Component generation    */
void pg_model_precalc_normals(struct pg_model* model)
{
    unsigned int t[3];
    int i;
    if(!(model->components & PG_MODEL_COMPONENT_POSITION)) return;
    model->components |= PG_MODEL_COMPONENT_NORMAL;
    ARR_RESERVE(model->normal, model->v_count);
    ARR_TRUNCATE_CLEAR(model->normal, 0);
    model->normal.len = model->v_count;
    for(i = 0; i < model->tris.len; i += 3) {
        t[0] = model->tris.data[i];
        t[1] = model->tris.data[i + 1];
        t[2] = model->tris.data[i + 2];
        /*  First we calculate the normal for each vertex   */
        vec3 norm;
        vec3 edge0, edge1;
        vec3_sub(edge0, model->pos.data[t[0]].v, model->pos.data[t[1]].v);
        vec3_sub(edge1, model->pos.data[t[0]].v, model->pos.data[t[2]].v);
        vec3_mul_cross(norm, edge0, edge1);
        vec3_normalize(norm, norm);
        /*  Now we average the calculated values with any existing values,
            so each vertex will have an average value for each associated
            face    */
        int j;
        for(j = 0; j < 3; ++j) {
            vec3_add(model->normal.data[t[j]].v, norm, model->normal.data[t[j]].v);
            vec3_normalize(model->normal.data[t[j]].v, model->normal.data[t[j]].v);
        }
    }
}

void pg_model_precalc_ntb(struct pg_model* model)
{
    unsigned int t[3];
    int i;
    if(!(model->components &
        (PG_MODEL_COMPONENT_POSITION
        | PG_MODEL_COMPONENT_UV))) return;
    model->components |=
        PG_MODEL_COMPONENT_NORMAL | PG_MODEL_COMPONENT_TAN_BITAN;
    ARR_RESERVE(model->normal, model->v_count);
    ARR_RESERVE(model->tangent, model->v_count);
    ARR_RESERVE(model->bitangent, model->v_count);
    ARR_TRUNCATE_CLEAR(model->normal, 0);
    ARR_TRUNCATE_CLEAR(model->tangent, 0);
    ARR_TRUNCATE_CLEAR(model->bitangent, 0);
    model->normal.len = model->v_count;
    model->tangent.len = model->v_count;
    model->bitangent.len = model->v_count;
    for(i = 0; i < model->tris.len; i += 3) {
        t[0] = model->tris.data[i];
        t[1] = model->tris.data[i + 1];
        t[2] = model->tris.data[i + 2];
        /*  First we calculate the normal for each vertex   */
        vec3 norm;
        vec3 edge0, edge1;
        vec3_sub(edge0, model->pos.data[t[0]].v, model->pos.data[t[1]].v);
        vec3_sub(edge1, model->pos.data[t[0]].v, model->pos.data[t[2]].v);
        vec3_mul_cross(norm, edge0, edge1);
        vec3_normalize(norm, norm);
        /*  Next we calculate the tangent and bitangent in alignment with the
            tex_coords  */
        vec2 tex_d0, tex_d1;
        vec2_sub(tex_d0, model->uv.data[t[1]].v, model->uv.data[t[0]].v);
        vec2_sub(tex_d1, model->uv.data[t[2]].v, model->uv.data[t[0]].v);
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
            vec3_add(model->normal.data[t[j]].v, model->normal.data[t[j]].v, norm);
            vec3_normalize(model->normal.data[t[j]].v, model->normal.data[t[j]].v);
            vec3_add(model->tangent.data[t[j]].v, model->tangent.data[t[j]].v,
                     tangent);
            vec3_normalize(model->tangent.data[t[j]].v,
                           model->tangent.data[t[j]].v);
            vec3_add(model->bitangent.data[t[j]].v, model->bitangent.data[t[j]].v,
                     bitangent);
            vec3_normalize(model->bitangent.data[t[j]].v,
                           model->bitangent.data[t[j]].v);
        }
    }
}

void pg_model_split_tris(struct pg_model* model)
{
    struct pg_model new_model;
    pg_model_init(&new_model);
    new_model.components = model->components;
    struct pg_vertex_full tmp;
    unsigned t[3];
    int i;
    for(i = 0; i < model->tris.len; i += 3) {
        t[0] = model->tris.data[i];
        t[1] = model->tris.data[i + 1];
        t[2] = model->tris.data[i + 2];
        pg_model_get_vertex(model, &tmp, t[0]);
        pg_model_add_vertex(&new_model, &tmp);
        pg_model_get_vertex(model, &tmp, t[1]);
        pg_model_add_vertex(&new_model, &tmp);
        pg_model_get_vertex(model, &tmp, t[2]);
        pg_model_add_vertex(&new_model, &tmp);
        pg_model_add_triangle(model, i, i + 1, i + 2);
    }
    pg_model_deinit(model);
    *model = new_model;
}

void pg_model_blend_duplicates(struct pg_model* model, float tolerance)
{
    if(!(model->components & (PG_MODEL_COMPONENT_POSITION |
                              PG_MODEL_COMPONENT_NORMAL)))
        return;
    int i, j;
    for(i = 0; i < model->v_count; ++i) {
        for(j = 0; j < model->v_count; ++j) {
            if(i == j) continue;
            if(fabsf(model->pos.data[i].v[0] - model->pos.data[j].v[0]) > 0.00001
            || fabsf(model->pos.data[i].v[1] - model->pos.data[j].v[1]) > 0.00001
            || fabsf(model->pos.data[i].v[2] - model->pos.data[j].v[2]) > 0.00001)
                continue;
            float angle = acosf(vec3_mul_inner(model->normal.data[i].v,
                                               model->normal.data[j].v));
            if(angle < tolerance) {
                vec3 norm, tan, bitan;
                vec3_add(norm, model->normal.data[i].v, model->normal.data[j].v);
                vec3_normalize(norm, norm);
                vec3_dup(model->normal.data[i].v, norm);
                vec3_dup(model->normal.data[j].v, norm);
                if(model->components & PG_MODEL_COMPONENT_TAN_BITAN) {
                    vec3_add(tan, model->tangent.data[i].v,
                             model->tangent.data[j].v);
                    vec3_add(bitan, model->bitangent.data[i].v,
                             model->bitangent.data[j].v);
                    vec3_normalize(tan, tan);
                    vec3_normalize(bitan, bitan);
                    vec3_dup(model->tangent.data[i].v, tan);
                    vec3_dup(model->tangent.data[j].v, tan);
                    vec3_dup(model->bitangent.data[i].v, bitan);
                    vec3_dup(model->bitangent.data[j].v, bitan);
                }
            }
        }
    }
}

static void pg_model_reset_buffers(struct pg_model* model)
{
    struct pg_model_buffer* buf;
    int i;
    ARR_FOREACH_PTR(model->buffers, buf, i) {
        int j;
        glDeleteVertexArrays(1, &buf->vao);
        buf->vao = 0;
        if(!buf->vbo) break;
        for(j = i + 1; j < model->buffers.len; ++j) {
            if(model->buffers.data[j].vbo == buf->vbo)
                model->buffers.data[j].vbo = 0;
        }
        glDeleteBuffers(1, &buf->vbo);
        buf->vbo = 0;
        buf->shader = NULL;
    }
    ARR_TRUNCATE(model->buffers, 0);
}

#if 0
void pg_model_generate_texture(struct pg_model* model,
                                   struct pg_texture* texture,
                                   unsigned w, unsigned h)
{
    struct pg_vert3d* v[3];
    unsigned t[3];
    int i;
    for(i = 0; i < model->tris.len; i += 3) {
        t[0] = model->tris.data[i].v;
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
#endif


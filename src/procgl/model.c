#include "procgl.h"

#if 0
void pg_vertex_transform(struct pg_model_vertex* out, struct pg_model_vertex* src,
                         mat4 transform)
{
    mat4 inv;
    mat4_invert(inv, transform);
    mat4 normal_matrix;
    mat4_transpose(normal_matrix, inv);
    int i;
    if(src->attribs[PG_ATTRIB_POSITION].used) {
        src->attribs[PG_ATTRIB_POSITION].v3.data
    if(src->components & PG_MODEL_COMPONENT_POSITION) {
        vec4 old = { src->pos[0], src->pos[1], src->pos[2], 1.0f };
        vec4 new;
        mat4_mul_vec4(new, transform, old);
        vec3_dup(out->pos, new);
    }
    if(src->components & PG_MODEL_COMPONENT_COLOR) {
        vec4_dup(out->color, src->color);
    }
    if(src->components & PG_MODEL_COMPONENT_UV) {
        vec2_dup(out->uv, src->uv);
    }
    if(src->components & PG_MODEL_COMPONENT_NORMAL) {
        vec4 old = { src->normal[0], src->normal[1], src->normal[2], 1.0f };
        vec4 new;
        mat4_mul_vec4(new, normal_matrix, old);
        vec3_dup(out->normal, new);
    }
    if(src->components & PG_MODEL_COMPONENT_TANGENT) {
        vec4 old = { src->tangent[0], src->tangent[1], src->tangent[2], 1.0f };
        vec4 new;
        mat4_mul_vec4(new, normal_matrix, old);
        vec3_dup(out->tangent, new);
    }
    if(src->components & PG_MODEL_COMPONENT_BITANGENT) {
        vec4 old = { src->bitangent[0], src->bitangent[1], src->bitangent[2], 1.0f };
        vec4 new;
        mat4_mul_vec4(new, normal_matrix, old);
        vec3_dup(out->bitangent, new);
    }
    if(src->components & PG_MODEL_COMPONENT_HEIGHT) {
        out->height = src->height;
    }
}
#endif

/*  Setup+cleanup   */
void pg_model_init(struct pg_model* model)
{
    *model = (struct pg_model){};
    int i;
    for(i = 0; i < PG_NUM_ATTRIBS; ++i) {
        model->attribs[i].type = PG_SHADER_ATTRIBUTE_INFO[i].type;
    }
}

static void reset_attrib_array(struct pg_model_attribute_array* arr)
{
    switch(arr->type) {
        case PG_BYTE: ARR_TRUNCATE(arr->b1, 0); break;
        case PG_BVEC2: ARR_TRUNCATE(arr->b2, 0); break;
        case PG_BVEC3: ARR_TRUNCATE(arr->b3, 0); break;
        case PG_BVEC4: ARR_TRUNCATE(arr->b4, 0); break;
        case PG_UBYTE: ARR_TRUNCATE(arr->ub1, 0); break;
        case PG_UBVEC2: ARR_TRUNCATE(arr->ub2, 0); break;
        case PG_UBVEC3: ARR_TRUNCATE(arr->ub3, 0); break;
        case PG_UBVEC4: ARR_TRUNCATE(arr->ub4, 0); break;
        case PG_INT: ARR_TRUNCATE(arr->i1, 0); break;
        case PG_IVEC2: ARR_TRUNCATE(arr->i2, 0); break;
        case PG_IVEC3: ARR_TRUNCATE(arr->i3, 0); break;
        case PG_IVEC4: ARR_TRUNCATE(arr->i4, 0); break;
        case PG_UINT: ARR_TRUNCATE(arr->u1, 0); break;
        case PG_UVEC2: ARR_TRUNCATE(arr->u2, 0); break;
        case PG_UVEC3: ARR_TRUNCATE(arr->u3, 0); break;
        case PG_UVEC4: ARR_TRUNCATE(arr->u4, 0); break;
        case PG_FLOAT: ARR_TRUNCATE(arr->f1, 0); break;
        case PG_VEC2: ARR_TRUNCATE(arr->f2, 0); break;
        case PG_VEC3: ARR_TRUNCATE(arr->f3, 0); break;
        case PG_VEC4: ARR_TRUNCATE(arr->f4, 0); break;
        default: break;
    }
}

static void deinit_attrib_array(struct pg_model_attribute_array* arr)
{
    switch(arr->type) {
        case PG_BYTE: ARR_DEINIT(arr->b1); break;
        case PG_BVEC2: ARR_DEINIT(arr->b2); break;
        case PG_BVEC3: ARR_DEINIT(arr->b3); break;
        case PG_BVEC4: ARR_DEINIT(arr->b4); break;
        case PG_UBYTE: ARR_DEINIT(arr->ub1); break;
        case PG_UBVEC2: ARR_DEINIT(arr->ub2); break;
        case PG_UBVEC3: ARR_DEINIT(arr->ub3); break;
        case PG_UBVEC4: ARR_DEINIT(arr->ub4); break;
        case PG_INT: ARR_DEINIT(arr->i1); break;
        case PG_IVEC2: ARR_DEINIT(arr->i2); break;
        case PG_IVEC3: ARR_DEINIT(arr->i3); break;
        case PG_IVEC4: ARR_DEINIT(arr->i4); break;
        case PG_UINT: ARR_DEINIT(arr->u1); break;
        case PG_UVEC2: ARR_DEINIT(arr->u2); break;
        case PG_UVEC3: ARR_DEINIT(arr->u3); break;
        case PG_UVEC4: ARR_DEINIT(arr->u4); break;
        case PG_FLOAT: ARR_DEINIT(arr->f1); break;
        case PG_VEC2: ARR_DEINIT(arr->f2); break;
        case PG_VEC3: ARR_DEINIT(arr->f3); break;
        case PG_VEC4: ARR_DEINIT(arr->f4); break;
        default: break;
    }
}

void pg_model_reset(struct pg_model* model)
{
    int i;
    for(i = 0; i < PG_NUM_ATTRIBS; ++i) {
        reset_attrib_array(&model->attribs[i]);
    }
    ARR_TRUNCATE(model->tris, 0);
    model->v_count = 0;
}

void pg_model_deinit(struct pg_model* model)
{
    int i;
    for(i = 0; i < PG_NUM_ATTRIBS; ++i) {
        deinit_attrib_array(&model->attribs[i]);
    }
    ARR_DEINIT(model->tris);
    model->v_count = 0;
    if(model->buf.ebo) glDeleteBuffers(1, &model->buf.ebo);
    if(model->buf.vbo) glDeleteBuffers(1, &model->buf.vbo);
    if(model->buf.vao) glDeleteVertexArrays(1, &model->buf.vao);
    if(model->buf.buffer) free(model->buf.buffer);
}

/*  Shader handling */
void pg_model_buffer(struct pg_model* model)
{
    int i;
    size_t vert_size = 0;
    size_t attr_offset[PG_NUM_ATTRIBS] = {};
    size_t attr_stride[PG_NUM_ATTRIBS] = {};
    for(i = 0; i < PG_NUM_ATTRIBS; ++i) {
        if(!model->attribs[i].used) continue;
        const struct pg_shader_attribute_info* attr_info = &PG_SHADER_ATTRIBUTE_INFO[i];
        attr_offset[i] = vert_size;
        vert_size += attr_info->size;
    }
    size_t buf_size = vert_size * model->v_count;
    model->buf.buffer = realloc(model->buf.buffer, buf_size);
    for(i = 0; i < PG_NUM_ATTRIBS; ++i) {
        if(!model->attribs[i].used) continue;
        printf("Copying attr %d to CPU buffer\n", i);
        const struct pg_shader_attribute_info* attr_info = &PG_SHADER_ATTRIBUTE_INFO[i];
        size_t data_size = attr_info->size;
        attr_stride[i] = vert_size;
        void* v_data;
        switch(attr_info->type) {
            case PG_BYTE: v_data = model->attribs[i].b1.data; break;
            case PG_BVEC2: v_data = model->attribs[i].b2.data; break;
            case PG_BVEC3: v_data = model->attribs[i].b3.data; break;
            case PG_BVEC4: v_data = model->attribs[i].b4.data; break;
            case PG_UBYTE: v_data = model->attribs[i].ub1.data; break;
            case PG_UBVEC2: v_data = model->attribs[i].ub2.data; break;
            case PG_UBVEC3: v_data = model->attribs[i].ub3.data; break;
            case PG_UBVEC4: v_data = model->attribs[i].ub4.data; break;
            case PG_INT: v_data = model->attribs[i].i1.data; break;
            case PG_IVEC2: v_data = model->attribs[i].i2.data; break;
            case PG_IVEC3: v_data = model->attribs[i].i3.data; break;
            case PG_IVEC4: v_data = model->attribs[i].i4.data; break;
            case PG_UINT: v_data = model->attribs[i].u1.data; break;
            case PG_UVEC2: v_data = model->attribs[i].u2.data; break;
            case PG_UVEC3: v_data = model->attribs[i].u3.data; break;
            case PG_UVEC4: v_data = model->attribs[i].u4.data; break;
            case PG_FLOAT: v_data = model->attribs[i].f1.data; break;
            case PG_VEC2: v_data = model->attribs[i].f2.data; break;
            case PG_VEC3: v_data = model->attribs[i].f3.data; break;
            case PG_VEC4: v_data = model->attribs[i].f4.data; break;
            default: v_data = NULL; break;
        }
        if(!v_data) continue;
        size_t buf_idx = attr_offset[i];
        size_t v_idx = 0;
        int j;
        for(j = 0; j < model->v_count; ++j) {
            memcpy(model->buf.buffer + buf_idx, v_data + v_idx, data_size);
            buf_idx += vert_size;
            v_idx += data_size;
        }
    }
    if(model->buf.ebo == 0) glGenBuffers(1, &model->buf.ebo);
    if(model->buf.vao == 0) glGenVertexArrays(1, &model->buf.vao);
    if(model->buf.vbo == 0) glGenBuffers(1, &model->buf.vbo);
    glBindVertexArray(model->buf.vao);
    glBindBuffer(GL_ARRAY_BUFFER, model->buf.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->buf.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 model->tris.len * sizeof(*model->tris.data),
                 model->tris.data, GL_STATIC_DRAW);
    printf("Copying vertex data to GPU: size: %zu\n", buf_size);
    glBufferData(GL_ARRAY_BUFFER, buf_size, model->buf.buffer, GL_STATIC_DRAW);
    for(i = 0; i < PG_NUM_ATTRIBS; ++i) {
        if(!model->attribs[i].used) continue;
        const struct pg_shader_attribute_info* attr_info = &PG_SHADER_ATTRIBUTE_INFO[i];
        if(attr_info->as_integer) {
            glVertexAttribIPointer(i, attr_info->elements, attr_info->gltype,
                                   attr_stride[i], (void*)attr_offset[i]);
        } else {
            glVertexAttribPointer(i, attr_info->elements,
                                  attr_info->gltype, attr_info->normalized,
                                  attr_stride[i], (void*)attr_offset[i]);
        }
        glEnableVertexAttribArray(i);
    }
    glBindVertexArray(0);
}

void pg_model_begin(struct pg_model* model, struct pg_shader* shader)
{
    glBindVertexArray(model->buf.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->buf.ebo);
}

void pg_model_draw(struct pg_model* model, mat4 transform)
{
/*
    if(model->active < 0) return;
    struct pg_shader* m_shader = model->buffers.data[model->active].shader;
    if(transform) {
        pg_shader_set_matrix(m_shader, PG_MODEL_MATRIX, transform);
        pg_shader_rebuild_matrices(m_shader);
    }
    glDrawElements(GL_TRIANGLES, model->tris.len * 3, GL_UNSIGNED_INT, 0);*/
}

/*  Raw vertex/triangle building    */

void pg_model_transform(struct pg_model* model, mat4 transform)
{
    int i;
    mat4 inv, normal_tx;
    mat4_invert(inv, transform);
    mat4_transpose(normal_tx, inv);
    vec3* v;
    if(model->attribs[PG_ATTRIB_POSITION].used) {
        for(i = 0; i < model->v_count; ++i) {
            v = &model->attribs[PG_ATTRIB_POSITION].f3.data[i].v;
            mat4_mul_vec3(*v, transform, *v);
        }
    }
    if(model->attribs[PG_ATTRIB_NORMAL].used) {
        for(i = 0; i < model->v_count; ++i) {
            v = &model->attribs[PG_ATTRIB_NORMAL].f3.data[i].v;
            mat3_mul_vec3(*v, normal_tx, *v);
        }
    }
    if(model->attribs[PG_ATTRIB_TANGENT].used) {
        for(i = 0; i < model->v_count; ++i) {
            v = &model->attribs[PG_ATTRIB_TANGENT].f3.data[i].v;
            mat3_mul_vec3(*v, normal_tx, *v);
        }
    }
    if(model->attribs[PG_ATTRIB_BITANGENT].used) {
        for(i = 0; i < model->v_count; ++i) {
            v = &model->attribs[PG_ATTRIB_BITANGENT].f3.data[i].v;
            mat3_mul_vec3(*v, normal_tx, *v);
        }
    }
}

void pg_model_reserve_attrib(struct pg_model* model,
                             enum pg_shader_attribute attr, unsigned count)
{
    const struct pg_shader_attribute_info* attr_info = 
        &PG_SHADER_ATTRIBUTE_INFO[attr];
    model->attribs[attr].used = 1;
    switch(attr_info->type) {
        case PG_BYTE: ARR_RESERVE_CLEAR(model->attribs[attr].b1, count); break;
        case PG_BVEC2: ARR_RESERVE_CLEAR(model->attribs[attr].b2, count); break;
        case PG_BVEC3: ARR_RESERVE_CLEAR(model->attribs[attr].b3, count); break;
        case PG_BVEC4: ARR_RESERVE_CLEAR(model->attribs[attr].b4, count); break;
        case PG_UBYTE: ARR_RESERVE_CLEAR(model->attribs[attr].ub1, count); break;
        case PG_UBVEC2: ARR_RESERVE_CLEAR(model->attribs[attr].ub2, count); break;
        case PG_UBVEC3: ARR_RESERVE_CLEAR(model->attribs[attr].ub3, count); break;
        case PG_UBVEC4: ARR_RESERVE_CLEAR(model->attribs[attr].ub4, count); break;
        case PG_INT: ARR_RESERVE_CLEAR(model->attribs[attr].i1, count); break;
        case PG_IVEC2: ARR_RESERVE_CLEAR(model->attribs[attr].i2, count); break;
        case PG_IVEC3: ARR_RESERVE_CLEAR(model->attribs[attr].i3, count); break;
        case PG_IVEC4: ARR_RESERVE_CLEAR(model->attribs[attr].i4, count); break;
        case PG_UINT: ARR_RESERVE_CLEAR(model->attribs[attr].u1, count); break;
        case PG_UVEC2: ARR_RESERVE_CLEAR(model->attribs[attr].u2, count); break;
        case PG_UVEC3: ARR_RESERVE_CLEAR(model->attribs[attr].u3, count); break;
        case PG_UVEC4: ARR_RESERVE_CLEAR(model->attribs[attr].u4, count); break;
        case PG_FLOAT: ARR_RESERVE_CLEAR(model->attribs[attr].f1, count); break;
        case PG_VEC2: ARR_RESERVE_CLEAR(model->attribs[attr].f2, count); break;
        case PG_VEC3: ARR_RESERVE_CLEAR(model->attribs[attr].f3, count); break;
        case PG_VEC4: ARR_RESERVE_CLEAR(model->attribs[attr].f4, count); break;
        default: break;
    }
}

void pg_model_reserve_verts(struct pg_model* model, unsigned count)
{
    int i;
    for(i = 0; i < PG_NUM_ATTRIBS; ++i) {
        if(!model->attribs[i].used) continue;
        pg_model_reserve_attrib(model, i, count);
    }
}

void pg_model_set_attrib(struct pg_model* model,
                         enum pg_shader_attribute attr, unsigned idx,
                         struct pg_model_attribute* set)
{
    const struct pg_shader_attribute_info* attr_info = 
        &PG_SHADER_ATTRIBUTE_INFO[attr];
    if(idx > model->v_count || !model->attribs[attr].used) return;
    switch(attr_info->type) {
        case PG_BYTE: model->attribs[attr].b1.data[idx] = set->b[0]; break;
        case PG_BVEC2: vec2_dup(model->attribs[attr].b2.data[idx].v, set->b); break;
        case PG_BVEC3: vec3_dup(model->attribs[attr].b3.data[idx].v, set->b); break;
        case PG_BVEC4: vec4_dup(model->attribs[attr].b4.data[idx].v, set->b); break;
        case PG_UBYTE: model->attribs[attr].ub1.data[idx] = set->ub[0]; break;
        case PG_UBVEC2: vec2_dup(model->attribs[attr].ub2.data[idx].v, set->ub); break;
        case PG_UBVEC3: vec3_dup(model->attribs[attr].ub3.data[idx].v, set->ub); break;
        case PG_UBVEC4: vec4_dup(model->attribs[attr].ub4.data[idx].v, set->ub); break;
        case PG_INT: model->attribs[attr].i1.data[idx] = set->i[0]; break;
        case PG_IVEC2: vec2_dup(model->attribs[attr].i2.data[idx].v, set->i); break;
        case PG_IVEC3: vec3_dup(model->attribs[attr].i3.data[idx].v, set->i); break;
        case PG_IVEC4: vec4_dup(model->attribs[attr].i4.data[idx].v, set->i); break;
        case PG_UINT: model->attribs[attr].u1.data[idx] = set->u[0]; break;
        case PG_UVEC2: vec2_dup(model->attribs[attr].u2.data[idx].v, set->u); break;
        case PG_UVEC3: vec3_dup(model->attribs[attr].u3.data[idx].v, set->u); break;
        case PG_UVEC4: vec4_dup(model->attribs[attr].u4.data[idx].v, set->u); break;
        case PG_FLOAT: model->attribs[attr].f1.data[idx] = set->i[0]; break;
        case PG_VEC2: vec2_dup(model->attribs[attr].f2.data[idx].v, set->f); break;
        case PG_VEC3: vec3_dup(model->attribs[attr].f3.data[idx].v, set->f); break;
        case PG_VEC4: vec4_dup(model->attribs[attr].f4.data[idx].v, set->f); break;
        default: break;
    }
}

void pg_model_reserve_tris(struct pg_model* model, unsigned count)
{
    ARR_RESERVE(model->tris, count);
    model->tris.len = count;
}

void pg_model_add_triangle(struct pg_model* model, unsigned v0,
                           unsigned v1, unsigned v2)
{
    struct pg_tri tri = { { v0, v1, v2 } };
    ARR_PUSH(model->tris, tri);
}

#if 0
void pg_model_reserve_verts(struct pg_model* model, unsigned count)
{
    model->v_count = count;
    if(model->components & PG_MODEL_COMPONENT_POSITION) {
        ARR_RESERVE_CLEAR(model->pos, count);
        model->pos.len = count;
    }
    if(model->components & PG_MODEL_COMPONENT_COLOR) {
        ARR_RESERVE_CLEAR(model->color, count);
        model->color.len = count;
    }
    if(model->components & PG_MODEL_COMPONENT_UV) {
        ARR_RESERVE_CLEAR(model->uv, count);
        model->uv.len = count;
    }
    if(model->components & PG_MODEL_COMPONENT_NORMAL) {
        ARR_RESERVE_CLEAR(model->normal, count);
        model->normal.len = count;
    }
    if(model->components & PG_MODEL_COMPONENT_TANGENT) {
        ARR_RESERVE_CLEAR(model->tangent, count);
        model->tangent.len = count;
    }
    if(model->components & PG_MODEL_COMPONENT_BITANGENT) {
        ARR_RESERVE_CLEAR(model->bitangent, count);
        model->bitangent.len = count;
    }
    if(model->components & PG_MODEL_COMPONENT_HEIGHT) {
        ARR_RESERVE_CLEAR(model->height, count);
        model->height.len = count;
    }
}

void pg_model_reserve_component(struct pg_model* model, uint32_t comp)
{
    if(comp & ~model->components & PG_MODEL_COMPONENT_POSITION) {
        ARR_RESERVE_CLEAR(model->pos, model->v_count);
        model->pos.len = model->v_count;
    }
    if(comp & ~model->components & PG_MODEL_COMPONENT_COLOR) {
        ARR_RESERVE_CLEAR(model->color, model->v_count);
        model->color.len = model->v_count;
    }
    if(comp & ~model->components & PG_MODEL_COMPONENT_UV) {
        ARR_RESERVE_CLEAR(model->uv, model->v_count);
        model->uv.len = model->v_count;
    }
    if(comp & ~model->components & PG_MODEL_COMPONENT_NORMAL) {
        ARR_RESERVE_CLEAR(model->normal, model->v_count);
        model->normal.len = model->v_count;
    }
    if(comp & ~model->components & PG_MODEL_COMPONENT_TANGENT) {
        ARR_RESERVE_CLEAR(model->tangent, model->v_count);
        model->tangent.len = model->v_count;
    }
    if(comp & ~model->components & PG_MODEL_COMPONENT_BITANGENT) {
        ARR_RESERVE_CLEAR(model->bitangent, model->v_count);
        model->bitangent.len = model->v_count;
    }
    if(comp & ~model->components & PG_MODEL_COMPONENT_HEIGHT) {
        ARR_RESERVE_CLEAR(model->height, model->v_count);
        model->height.len = model->v_count;
    }
    model->components |= comp;
}

void pg_model_set_vertex(struct pg_model* model, struct pg_model_vertex* v,
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

unsigned pg_model_add_vertex(struct pg_model* model, struct pg_model_vertex* v)
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

void pg_model_get_vertex(struct pg_model* model, struct pg_model_vertex* out,
                         unsigned i)
{
    if(i >= model->v_count) return;
    *out = (struct pg_model_vertex){ .components = model->components };
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

static void pg_model_remove_vertex(struct pg_model* model, unsigned v)
{
    if(v >= model->v_count) return;
    if(model->components & PG_MODEL_COMPONENT_POSITION) {
        ARR_SWAPSPLICE(model->pos, v, 1);
    }
    if(model->components & PG_MODEL_COMPONENT_COLOR) {
        ARR_SWAPSPLICE(model->color, v, 1);
    }
    if(model->components & PG_MODEL_COMPONENT_UV) {
        ARR_SWAPSPLICE(model->uv, v, 1);
    }
    if(model->components & PG_MODEL_COMPONENT_NORMAL) {
        ARR_SWAPSPLICE(model->normal, v, 1);
    }
    if(model->components & PG_MODEL_COMPONENT_TANGENT) {
        ARR_SWAPSPLICE(model->tangent, v, 1);
    }
    if(model->components & PG_MODEL_COMPONENT_BITANGENT) {
        ARR_SWAPSPLICE(model->bitangent, v, 1);
    }
    if(model->components & PG_MODEL_COMPONENT_HEIGHT) {
        ARR_SWAPSPLICE(model->height, v, 1);
    }
    --model->v_count;
    int i;
    struct pg_tri* tri;
    ARR_FOREACH_PTR_REV(model->tris, tri, i) {
        if(tri->t[0] == v || tri->t[1] == v || tri->t[2] == v)
            ARR_SWAPSPLICE(model->tris, i, 1);
        else if(tri->t[0] == model->v_count) tri->t[0] = v;
        else if(tri->t[1] == model->v_count) tri->t[1] = v;
        else if(tri->t[2] == model->v_count) tri->t[2] = v;
    }
}

/*  Compos/transformation  */
void pg_model_append(struct pg_model* dst, struct pg_model* src,
                     mat4 transform)
{
    unsigned dst_v = dst->v_count;
    struct pg_model_vertex src_vert, new_vert;
    int src_i;
    for(src_i = 0; src_i < src->v_count; ++src_i) {
        pg_model_get_vertex(src, &src_vert, src_i);
        pg_vertex_transform(&new_vert, &src_vert, transform);
        pg_model_add_vertex(dst, &new_vert);
    }
    ARR_RESERVE(dst->tris, dst->tris.len + src->tris.len);
    int i;
    struct pg_tri* tri;
    ARR_FOREACH_PTR(src->tris, tri, i) {
        struct pg_tri dtri =
            { { tri->t[0] + dst_v, tri->t[1] + dst_v, tri->t[2] + dst_v } };
        ARR_PUSH(dst->tris, dtri);
    }
}

void pg_model_invert_tris(struct pg_model* model)
{
    int i;
    struct pg_tri* tri;
    ARR_FOREACH_PTR(model->tris, tri, i) {
        unsigned tmp = tri->t[0];
        tri->t[0] = tri->t[1];
        tri->t[1] = tmp;
    }
}

/*  Component generation    */
void pg_model_precalc_normals(struct pg_model* model)
{
    if(!(model->components & PG_MODEL_COMPONENT_POSITION)) return;
    model->components |= PG_MODEL_COMPONENT_NORMAL;
    ARR_RESERVE_CLEAR(model->normal, model->v_count);
    model->normal.len = model->v_count;
    int i;
    struct pg_tri* tri;
    ARR_FOREACH_PTR(model->tris, tri, i) {
        vec3 norm, edge0, edge1;
        vec3_sub(edge0, model->pos.data[tri->t[0]].v, model->pos.data[tri->t[1]].v);
        vec3_sub(edge1, model->pos.data[tri->t[0]].v, model->pos.data[tri->t[2]].v);
        vec3_mul_cross(norm, edge0, edge1);
        vec3_add(model->normal.data[tri->t[0]].v, model->normal.data[tri->t[0]].v, norm);
        vec3_add(model->normal.data[tri->t[1]].v, model->normal.data[tri->t[1]].v, norm);
        vec3_add(model->normal.data[tri->t[2]].v, model->normal.data[tri->t[2]].v, norm);
    }
    vec3_t* norm;
    ARR_FOREACH_PTR(model->normal, norm, i) {
        vec3_normalize(norm->v, norm->v);
    }
}

void pg_model_precalc_ntb(struct pg_model* model)
{
    if(!(model->components &
        (PG_MODEL_COMPONENT_POSITION
        | PG_MODEL_COMPONENT_UV))) return;
    model->components |=
        PG_MODEL_COMPONENT_NORMAL | PG_MODEL_COMPONENT_TAN_BITAN;
    ARR_RESERVE_CLEAR(model->normal, model->v_count);
    ARR_RESERVE_CLEAR(model->tangent, model->v_count);
    ARR_RESERVE_CLEAR(model->bitangent, model->v_count);
    model->normal.len = model->v_count;
    model->tangent.len = model->v_count;
    model->bitangent.len = model->v_count;
    int i;
    struct pg_tri* tri;
    ARR_FOREACH_PTR(model->tris, tri, i) {
        /*  Calculate normals first   */
        vec3 norm, edge0, edge1;
        vec3_sub(edge0, model->pos.data[tri->t[0]].v, model->pos.data[tri->t[1]].v);
        vec3_sub(edge1, model->pos.data[tri->t[0]].v, model->pos.data[tri->t[2]].v);
        vec3_mul_cross(norm, edge0, edge1);
        /*  Then calculate tangent and bitangent, aligned with the UV coords */
        vec2 tex_d0, tex_d1;
        vec2_sub(tex_d0, model->uv.data[tri->t[0]].v, model->uv.data[tri->t[1]].v);
        vec2_sub(tex_d1, model->uv.data[tri->t[0]].v, model->uv.data[tri->t[2]].v);
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
        int j;
        for(j = 0; j < 3; ++j) {
            vec3_add(model->normal.data[tri->t[j]].v, model->normal.data[tri->t[j]].v,
                     norm);
            vec3_add(model->tangent.data[tri->t[j]].v, model->tangent.data[tri->t[j]].v,
                     tangent);
            vec3_add(model->bitangent.data[tri->t[j]].v, model->bitangent.data[tri->t[j]].v,
                     bitangent);
        }
    }
    for(i = 0; i < model->v_count; ++i) {
        vec3_normalize(model->normal.data[i].v, model->normal.data[i].v);
        vec3_normalize(model->tangent.data[i].v, model->tangent.data[i].v);
        vec3_normalize(model->bitangent.data[i].v, model->bitangent.data[i].v);
    }
}

void pg_model_seams_tris(struct pg_model* model)
{
    struct pg_model new_model;
    pg_model_init(&new_model);
    new_model.components = model->components;
    struct pg_model_vertex tmp;
    int i;
    struct pg_tri* tri;
    ARR_FOREACH_PTR(model->tris, tri, i) {
        pg_model_get_vertex(model, &tmp, tri->t[0]);
        pg_model_add_vertex(&new_model, &tmp);
        pg_model_get_vertex(model, &tmp, tri->t[1]);
        pg_model_add_vertex(&new_model, &tmp);
        pg_model_get_vertex(model, &tmp, tri->t[2]);
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

void pg_model_warp_verts(struct pg_model* model)
{
    int i;
    for(i = 0; i < model->pos.len; ++i) {
        vec3 move = { rand() % 10 * 0.00001, rand() % 10 * 0.00001, rand() % 10 * 0.00001 };
        vec3_add(model->pos.data[i].v, model->pos.data[i].v, move);
    }
}

/*  This is bad. It needs to be replaced with a proper decimation function  */
void pg_model_join_duplicates(struct pg_model* model, float t)
{
    int i, j, k;
    struct pg_tri* tri;
    for(i = 0; i < model->v_count; ++i) {
        for(j = 0; j < model->v_count; ++j) {
            if(j == i
            || fabsf(model->pos.data[i].v[0] - model->pos.data[j].v[0]) > t
            || fabsf(model->pos.data[i].v[1] - model->pos.data[j].v[1]) > t
            || fabsf(model->pos.data[i].v[2] - model->pos.data[j].v[2]) > t)
                continue;
            ARR_FOREACH_PTR_REV(model->tris, tri, k) {
                int l = (tri->t[0] == j) ? 0 :
                        (tri->t[1] == j) ? 1 :
                        (tri->t[2] == j) ? 2 : 3;
                if(l == 3) continue;
                if(tri->t[0] == i || tri->t[1] == i || tri->t[2] == i) {
                    ARR_SWAPSPLICE(model->tris, k, 1);
                } else {
                    tri->t[l] = i;
                }
            }
            pg_model_remove_vertex(model, j);
            if(i > j) --i;
            --j;
        }
    }
}

void pg_model_get_face_normal(struct pg_model* model, unsigned t, vec3 out)
{
    struct pg_tri* tri = &model->tris.data[t];
    /*  First we calculate the normal for each vertex   */
    vec3 edge0, edge1;
    vec3_sub(edge0, model->pos.data[tri->t[0]].v, model->pos.data[tri->t[1]].v);
    vec3_sub(edge1, model->pos.data[tri->t[0]].v, model->pos.data[tri->t[2]].v);
    vec3_mul_cross(out, edge0, edge1);
    vec3_normalize(out, out);
}

/*  Collision functions */
static int nearest_on_triangle(vec3 out, vec3 const p,
                                vec3 const a, vec3 const b, vec3 const c)
{
    // Check if P in vertex region outside A
    vec3 ab, ac, ap, bp, cp;
    vec3_sub(ab, b, a);
    vec3_sub(ac, c, a);
    vec3_sub(ap, p, a);
    float d1 = vec3_mul_inner(ab, ap);
    float d2 = vec3_mul_inner(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) {
        vec3_dup(out, a);
        return 0;
    }
    // Check if P in vertex region outside B
    vec3_sub(bp, p, b);
    float d3 = vec3_mul_inner(ab, bp);
    float d4 = vec3_mul_inner(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) {
        vec3_dup(out, b);
        return 0; // barycentric coordinates (0,1,0)
    }
    // Check if P in vertex region outside C
    vec3_sub(cp, p, c);
    float d5 = vec3_mul_inner(ab, cp);
    float d6 = vec3_mul_inner(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) {
        vec3_dup(out, c);
        return 0; // barycentric coordinates (0,0,1)
    }
    // Check if P in edge region of AB, if so return projection of P onto AB
    float vc = d1*d4 - d3*d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v=d1/(d1- d3);
        vec3_dup(out, ab);
        vec3_scale(out, out, v);
        vec3_add(out, out, a);
        return 0; // barycentric coordinates (1-v,v,0)
    }
    // Check if P in edge region of AC, if so return projection of P onto AC
    float vb = d5*d2 - d1*d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w=d2/(d2- d6);
        vec3_dup(out, ac);
        vec3_scale(out, out, w);
        vec3_add(out, out, a);
        return 0; // barycentric coordinates (1-w,0,w)
    }
    // Check if P in edge region of BC, if so return projection of P onto BC
    float va = d3*d6 - d5*d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        vec3_sub(out, c, b);
        vec3_scale(out, out, w);
        vec3_add(out, out, b);
        return 0; // barycentric coordinates (0,1-w,w)
    }
    // P inside face region. Compute Q through its barycentric coordinates (u,v,w)
    float denom = 1.0f / (va + vb + vc);
    float v=vb* denom;
    float w=vc* denom;
    vec3_scale(ab, ab, v);
    vec3_scale(ac, ac, w);
    vec3_add(ab, ab, ac);
    vec3_add(out, ab, a);
    return 1;
}

int pg_model_collide_sphere(struct pg_model* model, vec3 out, vec3 const pos,
                            float r, int n)
{
    return pg_model_collide_sphere_sub(model, out, pos, r, n, 0, model->tris.len);
}

int pg_model_collide_sphere_sub(struct pg_model* model, vec3 out, vec3 const pos,
                                float r, int n, unsigned sub_i, unsigned sub_len)
{
    float deepest = 0;
    int tri_idx = -1;
    int sub_end = sub_i + sub_len;
    int i;
    int hits = 0;
    vec3 tri_push = {};
    for(i = sub_i; i < sub_end; ++i) {
        struct pg_tri* tri = &model->tris.data[i];
        vec3 p0, p1, p2;
        vec3_dup(p0, model->pos.data[tri->t[0]].v);
        vec3_dup(p1, model->pos.data[tri->t[1]].v);
        vec3_dup(p2, model->pos.data[tri->t[2]].v);
        vec3 pos_to_tri;
        int on_tri = nearest_on_triangle(pos_to_tri, pos, p0, p1, p2);
        vec3_sub(pos_to_tri, pos, pos_to_tri);
        if(on_tri) {
            vec3 norm;
            pg_model_get_face_normal(model, i, norm);
            if(vec3_mul_inner(pos_to_tri, norm) <= 0) continue;
        }
        float dist = vec3_len(pos_to_tri);
        if(dist < r) {
            /*  If this collision has the greatest depth so far, then
                set the result to this one  */
            float depth = r - dist;
            if(depth <= deepest) continue;
            deepest = depth;
            vec3_set_len(tri_push, pos_to_tri, depth);
            tri_idx = i;
            if(++hits >= n) break;
        }
    }
    vec3_dup(out, tri_push);
    return tri_idx;
}

int pg_model_collide_ellipsoid(struct pg_model* model, vec3 out, vec3 const pos,
                               vec3 const r, int n)
{
    return pg_model_collide_ellipsoid_sub(model, out, pos, r, n, 0, model->tris.len);
}

int pg_model_collide_ellipsoid_sub(struct pg_model* model, vec3 out, vec3 const pos,
                                   vec3 const r, int n, unsigned sub_i, unsigned sub_len)
{
    float deepest = 1;
    int tri_idx = -1;
    int sub_end = sub_i + sub_len;
    int i;
    int hits = 0;
    vec3 sphere_pos, tri_push = {};
    vec3_div(sphere_pos, pos, r);
    for(i = sub_i; i < sub_end; ++i) {
        struct pg_tri* tri = &model->tris.data[i];
        vec3 p0, p1, p2;
        vec3_dup(p0, model->pos.data[tri->t[0]].v);
        vec3_dup(p1, model->pos.data[tri->t[1]].v);
        vec3_dup(p2, model->pos.data[tri->t[2]].v);
        vec3_div(p0, p0, r);
        vec3_div(p1, p1, r);
        vec3_div(p2, p2, r);
        vec3 pos_to_tri;
        int on_tri = nearest_on_triangle(pos_to_tri, sphere_pos, p0, p1, p2);
        vec3_sub(pos_to_tri, sphere_pos, pos_to_tri);
        vec3 norm, pos_norm;
        vec3_normalize(pos_norm, pos_to_tri);
        pg_model_get_face_normal(model, i, norm);
        if(vec3_mul_inner(pos_norm, norm) < 0) continue;
        float dist = vec3_len(pos_to_tri);
        if(dist <= 1) {
            /*  If this collision has the greatest depth so far, then
                set the result to this one  */
            if(dist > deepest) continue;
            deepest = dist;
            //vec3_set_len(pos_to_tri, pos_to_tri, 1 - dist);
            //vec3_add(tri_push, tri_push, pos_to_tri);
            vec3_set_len(tri_push, pos_to_tri, 1 - dist);
            tri_idx = i;
            ++hits;
            if(++hits >= n) break;
        }
    }
    if(!hits) return -1;
    vec3 r2 = { r[0] * r[0], r[1] * r[1], r[2] * r[2] };
    vec3_mul(out, tri_push, r);
    //vec3_scale(out, out, 1 / hits);
    //vec3_div(out, tri_push, r2);
    return tri_idx;
}

#endif

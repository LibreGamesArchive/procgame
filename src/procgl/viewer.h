struct pg_viewer {
    vec2 size;
    vec2 near_far;
    vec3 pos;
    vec2 dir;
    mat4 view_matrix;
    mat4 proj_matrix;
};

void pg_viewer_init(struct pg_viewer* view, vec3 pos, vec2 dir,
                    vec2 size, vec2 near_far);
void pg_viewer_set(struct pg_viewer* view, vec3 pos, vec2 dir);
void pg_viewer_project(struct pg_viewer* view, vec2 out, vec3 const pos);

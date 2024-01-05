struct FrameData
{
    mat4 view;
    mat4 proj;
    mat4 view_proj;
    mat4 inv_view_proj;
    vec4 eye_pos_ws;
    float time; /* Time in seconds */
};

struct InstanceData
{
    mat4  model;
    vec4  color;
};
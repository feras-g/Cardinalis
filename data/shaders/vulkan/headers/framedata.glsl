struct FrameData
{
    mat4 view_proj;
    mat4 inv_view_proj;
    vec4 camera_pos_ws;
    float time; /* Time in seconds */
};
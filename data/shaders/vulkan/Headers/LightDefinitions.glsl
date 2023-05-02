struct PointLight
{
    vec4 position;
    vec4 color;
    vec4 props;
};

struct DirectionalLight
{
    vec4 direction;
    vec4 color;
    mat4 view_proj;
};
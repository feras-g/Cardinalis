struct Vertex
{
    float px,py,pz; // Important to keep as float, not vec* or vertex pulling won't work
    float nx,ny,nz;
    float u, v;
    float pad;
};

struct InstanceData
{
    mat4 model;
};


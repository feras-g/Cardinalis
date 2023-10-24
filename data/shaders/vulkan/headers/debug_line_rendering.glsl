struct VkDrawIndirectCommand 
{
    uint    vertexCount;
    uint    instanceCount;
    uint    firstVertex;
    uint    firstInstance;
};

struct DebugPoint
{
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 0) buffer DebugLinesVertexBuffer { DebugPoint data[]; } vertex_buffer;
layout(set = 0, binding = 1) buffer DebugLinesCommand { VkDrawIndirectCommand data; } command;

void DrawLine(in DebugPoint p0, in DebugPoint p1)
{
    uint vertex_offset = atomicAdd(command.data.vertexCount, 2);
    vertex_buffer.data[vertex_offset + 0]     = p0;
    vertex_buffer.data[vertex_offset + 1] = p1;
}

void DrawAxisAlignedBox(vec3 center, vec3 color, vec3 extent)
{
    const vec3 a = center - (vec3(-0.5, -0.5, 0.0) * extent);
    const vec3 b = center - (vec3( 0.5, -0.5, 0.0) * extent);
    const vec3 c = center - (vec3(-0.5, -0.5, 0.5) * extent);
    const vec3 d = center - (vec3( 0.5, -0.5, 0.5) * extent);
    const vec3 e = center - (vec3(-0.5, 0.5, 0.0)  * extent);
    const vec3 f = center - (vec3( 0.5, 0.5, 0.0)  * extent);
    const vec3 g = center - (vec3(-0.5, 0.5, 0.5)  * extent);
    const vec3 h = center - (vec3( 0.5, 0.5, 0.5)  * extent);

    DebugPoint pa;
    pa.position = vec4(a, 1.0f);
    pa.color    = vec4(color, 1.0f);

    DebugPoint pb;
    pb.position = vec4(b, 1.0f);
    pb.color    = vec4(color, 1.0f);

    DebugPoint pc;
    pc.position = vec4(c, 1.0f);
    pc.color    = vec4(color, 1.0f);

    DebugPoint pd;
    pd.position = vec4(d, 1.0f);
    pd.color    = vec4(color, 1.0f);

    DebugPoint pe;
    pe.position = vec4(e, 1.0f);
    pe.color    = vec4(color, 1.0f);

    DebugPoint pf;
    pf.position = vec4(f, 1.0f);
    pf.color    = vec4(color, 1.0f);

    DebugPoint pg;
    pg.position = vec4(g, 1.0f);
    pg.color    = vec4(color, 1.0f);

    DebugPoint ph;
    ph.position = vec4(h, 1.0f);
    ph.color    = vec4(color, 1.0f);

    DrawLine(pa, pb);
    DrawLine(pa, pc);
    DrawLine(pb, pd);
    DrawLine(pc, pd);
    DrawLine(pa, pe);
    DrawLine(pb, pf);
    DrawLine(pe, pf);
    DrawLine(pe, pg);
    DrawLine(pf, ph);
    DrawLine(pg, ph);
    DrawLine(pc, pg);
    DrawLine(pd, ph);
}
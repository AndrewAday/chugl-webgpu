#pragma once

struct Vertex {
    f32 x, y, z;    // position
    f32 nx, ny, nz; // normal
    f32 u, v;       // uv

    // TODO: static assert offsets match the strides
    // use these offsets to calculate strides when creating vertex buffers

    static void pos(Vertex* v, char c, f32 val);
    static void norm(Vertex* v, char c, f32 val);
};

struct Vertices {
    u32 vertexCount;
    u32 indicesCount;
    f32* vertexData; // alloc. owned
    u32* indices;    // alloc. owned

    // vertex data stored in single contiguous array
    // [positions | normals | texcoords]

    static f32* positions(Vertices* v);
    static f32* normals(Vertices* v);
    static f32* texcoords(Vertices* v);

    static void init(Vertices* v, u32 vertexCount, u32 indicesCount);
    static void setVertex(Vertices* vertices, Vertex v, u32 index);
    static void setIndices(Vertices* vertices, u32 a, u32 b, u32 c, u32 index);
    static void free(Vertices* v);
    static void print(Vertices* v);

    // copy from existing arrays
    static void copy(Vertices* v, Vertex* vertices, u32 vertexCount,
                     u32* indices, u32 indicesCount);
};

struct PlaneParams {
    f32 width, height;
    u32 widthSegments, heightSegments;
};

Vertices createPlane(const PlaneParams* params);
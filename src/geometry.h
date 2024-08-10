#pragma once

// tangents follow glTF 2.0 spec
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes
// see table on vertex attributes
// TODO add tangent (vec4)
// XYZW vertex tangents where the XYZ portion is normalized, and the W component
// is a sign value (-1 or +1) indicating handedness of the tangent basis
// unity code for calculating tangents
// https://discussions.unity.com/t/calculating-tangents-for-the-mesh/321060

// https://github.com/mmikk/MikkTSpace
// tangent space calculation
struct Vertex {
    f32 x, y, z;    // position
    f32 nx, ny, nz; // normal
    f32 u, v;       // uv

    // TODO: static assert offsets match the strides
    // use these offsets to calculate strides when creating vertex buffers

    static void pos(Vertex* v, char c, f32 val);
    static void norm(Vertex* v, char c, f32 val);
};

struct PlaneParams {
    f32 width = 1.0f, height = 1.0f;
    u32 widthSegments = 1, heightSegments = 1;
};

struct SphereParams {
    f32 radius   = 0.5;
    u32 widthSeg = 32, heightSeg = 16;
    f32 phiStart = 0.0, phiLength = 2 * PI; // how much along circumference
    f32 thetaStart = 0.0, thetaLength = PI; // how much along central diameter
};

// Arena builders
struct Arena;

struct GeometryArenaBuilder {
    Arena* pos_arena;
    Arena* norm_arena;
    Arena* uv_arena;
    Arena* tangent_arena;
    Arena* indices_arena;
};

void Geometry_buildPlane(GeometryArenaBuilder* builder, PlaneParams* params);
void Geometry_buildSphere(GeometryArenaBuilder* builder, SphereParams* params);
void Geometry_buildSuzanne(GeometryArenaBuilder* builder);

// TODO: redesign to support tangents / colors
// add hasColor / hasTangent / has... flags
// probably requires splitting vertex data into separate arrays
// per attribute type
#define CHUGL_FLOATS_PER_VERTEX (3 + 3 + 2 + 4)
struct Vertices {
    u32 vertexCount;
    u32 indicesCount;
    f32* vertexData; // alloc. owned
    u32* indices;    // alloc. owned

    // vertex data stored in single contiguous array
    // [positions | normals | texcoords | tangents]

    static f32* positions(Vertices* v);
    static f32* normals(Vertices* v);
    static f32* texcoords(Vertices* v);
    static f32* tangents(Vertices* v);

    static void buildTangents(Vertices* v);

    // returns offsets to contiguous attribute arrays in bytes
    static size_t positionOffset(Vertices* v);
    static size_t normalOffset(Vertices* v);
    static size_t texcoordOffset(Vertices* v);
    static size_t tangentOffset(Vertices* v);

    static void init(Vertices* v, u32 vertexCount, u32 indicesCount);
    static void setVertex(Vertices* vertices, Vertex v, u32 index);
    static void setIndices(Vertices* vertices, u32 a, u32 b, u32 c, u32 index);
    static void free(Vertices* v);
    static void print(Vertices* v);

    // copy from existing arrays
    static void copy(Vertices* v, Vertex* vertices, u32 vertexCount, u32* indices,
                     u32 indicesCount);

    // shapes
    static void createPlane(Vertices* vertices, PlaneParams* params);
    static void createSphere(Vertices* vertices, SphereParams* params);
};

// Vertices createPlane(const PlaneParams* params);

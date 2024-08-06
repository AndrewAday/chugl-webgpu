#include "geometry.h"

#include "core/memory.h"

#include <glm/gtc/epsilon.hpp>

#include <mikktspace/mikktspace.h>

#include <vector> // ew

// ============================================================================
// Vertex
// ============================================================================

void Vertex::pos(Vertex* v, char c, f32 val)
{
    switch (c) {
        case 'x': v->x = val; return;
        case 'y': v->y = val; return;
        case 'z': v->z = val; return;
        default: ASSERT(false); return; // error
    }
}

void Vertex::norm(Vertex* v, char c, f32 val)
{
    switch (c) {
        case 'x': v->nx = val; return;
        case 'y': v->ny = val; return;
        case 'z': v->nz = val; return;
        default: ASSERT(false); return; // error
    }
}

// ============================================================================
// Vertices
// ============================================================================

void Vertices::init(Vertices* v, u32 vertexCount, u32 indicesCount)
{

    ASSERT(indicesCount % 3 == 0); // must be a multiple of 3 triangles only
    ASSERT(v->vertexCount == 0);
    ASSERT(v->indicesCount == 0);

    v->vertexCount  = vertexCount;
    v->indicesCount = indicesCount;
    // TODO: change when adding more vertex attributes
    if (vertexCount > 0)
        v->vertexData = ALLOCATE_COUNT(f32, vertexCount * CHUGL_FLOATS_PER_VERTEX);
    if (indicesCount > 0) v->indices = ALLOCATE_COUNT(u32, indicesCount);
}

void Vertices::print(Vertices* v)
{

    f32* positions = Vertices::positions(v);
    f32* normals   = Vertices::normals(v);
    f32* texcoords = Vertices::texcoords(v);

    printf("Vertices: %d\n", v->vertexCount);
    printf("Indices: %d\n", v->indicesCount);
    // print arrays
    printf("Positions:\n");
    for (u32 i = 0; i < v->vertexCount; i++) {
        printf("\t%f %f %f\n", positions[i * 3 + 0], positions[i * 3 + 1],
               positions[i * 3 + 2]);
    }
    printf("Normals:\n");
    for (u32 i = 0; i < v->vertexCount; i++) {
        printf("\t%f %f %f\n", normals[i * 3 + 0], normals[i * 3 + 1],
               normals[i * 3 + 2]);
    }
    printf("Texcoords:\n");
    for (u32 i = 0; i < v->vertexCount; i++) {
        printf("\t%f %f\n", texcoords[i * 2 + 0], texcoords[i * 2 + 1]);
    }
    printf("Indices:\n");
    for (u32 i = 0; i < v->indicesCount; i += 3) {
        printf("\t%d %d %d\n", v->indices[i], v->indices[i + 1], v->indices[i + 2]);
    }
}

f32* Vertices::positions(Vertices* v)
{
    return v->vertexData;
}

f32* Vertices::normals(Vertices* v)
{
    return v->vertexData + v->vertexCount * 3;
}

f32* Vertices::texcoords(Vertices* v)
{
    return v->vertexData + v->vertexCount * 6;
}

f32* Vertices::tangents(Vertices* v)
{
    return v->vertexData + v->vertexCount * 8;
}

void Vertices::buildTangents(Vertices* v)
{
    SMikkTSpaceInterface mikktspaceIface = {};
    SMikkTSpaceContext mikktspaceContext = {};
    mikktspaceContext.m_pInterface       = &mikktspaceIface;
    mikktspaceContext.m_pUserData        = v;

    mikktspaceIface.m_getNumFaces = [](const SMikkTSpaceContext* pContext) {
        Vertices* v = (Vertices*)pContext->m_pUserData;
        return (int)(v->indicesCount > 0 ? v->indicesCount / 3 : v->vertexCount / 3);
    };

    mikktspaceIface.m_getNumVerticesOfFace
      = [](const SMikkTSpaceContext* pContext, const int iFace) {
            return 3; // triangles only
        };

    mikktspaceIface.m_getPosition
      = [](const SMikkTSpaceContext* pContext, f32 fvPosOut[], const int iFace,
           const int iVert) {
            Vertices* v    = (Vertices*)pContext->m_pUserData;
            u32 index      = (v->indicesCount > 0) ? v->indices[iFace * 3 + iVert] :
                                                     (iFace * 3 + iVert);
            f32* positions = Vertices::positions(v);
            fvPosOut[0]    = positions[index * 3 + 0];
            fvPosOut[1]    = positions[index * 3 + 1];
            fvPosOut[2]    = positions[index * 3 + 2];
        };

    mikktspaceIface.m_getNormal
      = [](const SMikkTSpaceContext* pContext, f32 fvNormOut[], const int iFace,
           const int iVert) {
            Vertices* v  = (Vertices*)pContext->m_pUserData;
            u32 index    = (v->indicesCount > 0) ? v->indices[iFace * 3 + iVert] :
                                                   (iFace * 3 + iVert);
            f32* normals = Vertices::normals(v);
            fvNormOut[0] = normals[index * 3 + 0];
            fvNormOut[1] = normals[index * 3 + 1];
            fvNormOut[2] = normals[index * 3 + 2];
        };

    mikktspaceIface.m_getTexCoord
      = [](const SMikkTSpaceContext* pContext, f32 fvTexcOut[], const int iFace,
           const int iVert) {
            Vertices* v    = (Vertices*)pContext->m_pUserData;
            u32 index      = (v->indicesCount > 0) ? v->indices[iFace * 3 + iVert] :
                                                     (iFace * 3 + iVert);
            f32* texcoords = Vertices::texcoords(v);
            fvTexcOut[0]   = texcoords[index * 2 + 0];
            fvTexcOut[1]   = texcoords[index * 2 + 1];
        };

    mikktspaceIface.m_setTSpaceBasic
      = [](const SMikkTSpaceContext* pContext, const f32 fvTangent[], const f32 fSign,
           const int iFace, const int iVert) {
            Vertices* v   = (Vertices*)pContext->m_pUserData;
            u32 index     = (v->indicesCount > 0) ? v->indices[iFace * 3 + iVert] :
                                                    (iFace * 3 + iVert);
            f32* tangents = Vertices::tangents(v);
            tangents[index * 4 + 0] = fvTangent[0];
            tangents[index * 4 + 1] = fvTangent[1];
            tangents[index * 4 + 2] = fvTangent[2];
            tangents[index * 4 + 3] = fSign;
        };

    genTangSpaceDefault(&mikktspaceContext);
}

size_t Vertices::positionOffset(Vertices* v)
{
    return 0;
}

size_t Vertices::normalOffset(Vertices* v)
{
    return v->vertexCount * 3 * sizeof(f32);
}

size_t Vertices::texcoordOffset(Vertices* v)
{
    return v->vertexCount * 6 * sizeof(f32);
}

size_t Vertices::tangentOffset(Vertices* v)
{
    return v->vertexCount * 8 * sizeof(f32);
}

void Vertices::setVertex(Vertices* vertices, Vertex v, u32 index)
{
    f32* positions = Vertices::positions(vertices);
    f32* normals   = Vertices::normals(vertices);
    f32* texcoords = Vertices::texcoords(vertices);

    // TODO: add tangents here?

    positions[index * 3 + 0] = v.x;
    positions[index * 3 + 1] = v.y;
    positions[index * 3 + 2] = v.z;

    normals[index * 3 + 0] = v.nx;
    normals[index * 3 + 1] = v.ny;
    normals[index * 3 + 2] = v.nz;

    texcoords[index * 2 + 0] = v.u;
    texcoords[index * 2 + 1] = v.v;
}

void Vertices::setIndices(Vertices* vertices, u32 a, u32 b, u32 c, u32 index)
{
    vertices->indices[index * 3 + 0] = a;
    vertices->indices[index * 3 + 1] = b;
    vertices->indices[index * 3 + 2] = c;
}

void Vertices::free(Vertices* v)
{
    reallocate(v->vertexData, sizeof(Vertex) * v->vertexCount, 0);
    reallocate(v->indices, sizeof(u32) * v->indicesCount, 0);
    v->vertexData   = NULL;
    v->indices      = NULL;
    v->vertexCount  = 0;
    v->indicesCount = 0;
}

void Vertices::copy(Vertices* v, Vertex* vertices, u32 vertexCount, u32* indices,
                    u32 indicesCount)
{
    Vertices::init(v, vertexCount, indicesCount);
    f32* positions = Vertices::positions(v);
    f32* normals   = Vertices::normals(v);
    f32* texcoords = Vertices::texcoords(v);

    for (u32 i = 0; i < vertexCount; i++) {
        // no memcpy here in case of compiler adding padding
        positions[i * 3 + 0] = vertices[i].x;
        positions[i * 3 + 1] = vertices[i].y;
        positions[i * 3 + 2] = vertices[i].z;

        normals[i * 3 + 0] = vertices[i].nx;
        normals[i * 3 + 1] = vertices[i].ny;
        normals[i * 3 + 2] = vertices[i].nz;

        texcoords[i * 2 + 0] = vertices[i].u;
        texcoords[i * 2 + 1] = vertices[i].v;

        // TODO tangents
    }

    memcpy(v->indices, indices, indicesCount * sizeof(u32));
}

// ============================================================================
// Plane
// ============================================================================
void Vertices::createPlane(Vertices* vertices, PlaneParams* params)
{
    ASSERT(vertices->vertexCount == 0);
    ASSERT(vertices->indicesCount == 0);

    const f32 width_half  = params->width * 0.5f;
    const f32 height_half = params->height * 0.5f;

    const u32 gridX = params->widthSegments;
    const u32 gridY = params->heightSegments;

    const u32 gridX1 = gridX + 1;
    const u32 gridY1 = gridY + 1;

    const f32 segment_width  = params->width / gridX;
    const f32 segment_height = params->height / gridY;

    Vertices::init(vertices, gridX1 * gridY1, gridX * gridY * 6);

    // f32* positions = Vertices::positions(&vertices);
    // f32* normals   = Vertices::normals(&vertices);
    // f32* texcoords = Vertices::texcoords(&vertices);

    u32 index   = 0;
    Vertex vert = {};
    for (u32 iy = 0; iy < gridY1; iy++) {
        const f32 y = iy * segment_height - height_half;
        for (u32 ix = 0; ix < gridX1; ix++) {
            const f32 x = ix * segment_width - width_half;

            vert = {
                x,  // x
                -y, // y
                0,  // z

                0, // nx
                0, // ny
                1, // nz

                (f32)ix / gridX,          // u
                1.0f - ((f32)iy / gridY), // v
            };

            Vertices::setVertex(vertices, vert, index++);
        }
    }
    ASSERT(index == vertices->vertexCount);
    index = 0;
    for (u32 iy = 0; iy < gridY; iy++) {
        for (u32 ix = 0; ix < gridX; ix++) {
            const u32 a = ix + gridX1 * iy;
            const u32 b = ix + gridX1 * (iy + 1);
            const u32 c = (ix + 1) + gridX1 * (iy + 1);
            const u32 d = (ix + 1) + gridX1 * iy;

            Vertices::setIndices(vertices, a, b, d, index++);
            Vertices::setIndices(vertices, b, c, d, index++);
        }
    }
    ASSERT(index == vertices->indicesCount / 3);
}

void Vertices::createSphere(Vertices* vertices, SphereParams* params)
{
    ASSERT(vertices->vertexCount == 0);
    ASSERT(vertices->indicesCount == 0);

    params->widthSeg  = MAX(3, params->widthSeg);
    params->heightSeg = MAX(2, params->heightSeg);

    const f32 thetaEnd = MIN(params->thetaStart + params->thetaLength, PI);

    u32 index = 0;

    std::vector<u32> grid;
    std::vector<Vertex> verts;
    std::vector<u32> indices;

    // generate vertices, normals and uvs
    for (u32 iy = 0; iy <= params->heightSeg; iy++) {

        const f32 v = (f32)iy / (f32)params->heightSeg;

        // special case for the poles
        f32 uOffset = 0;
        if (iy == 0 && glm::epsilonEqual(params->thetaStart, 0.0f, EPSILON)) {
            uOffset = 0.5f / params->widthSeg;
        } else if (iy == params->heightSeg
                   && glm::epsilonEqual(thetaEnd, PI, EPSILON)) {
            uOffset = -0.5 / params->widthSeg;
        }

        for (u32 ix = 0; ix <= params->widthSeg; ix++) {

            const f32 u = (f32)ix / (f32)params->widthSeg;

            Vertex vert;

            // vertex
            vert.x = -params->radius
                     * glm::cos(params->phiStart + u * params->phiLength)
                     * glm::sin(params->thetaStart + v * params->thetaLength);
            vert.y
              = params->radius * glm::cos(params->thetaStart + v * params->thetaLength);
            vert.z = params->radius * glm::sin(params->phiStart + u * params->phiLength)
                     * glm::sin(params->thetaStart + v * params->thetaLength);

            // normal
            glm::vec3 normal = glm::normalize(glm::vec3(vert.x, vert.y, vert.z));
            vert.nx          = normal.x;
            vert.ny          = normal.y;
            vert.nz          = normal.z;

            // uv
            vert.u = u + uOffset;
            vert.v = 1 - v;

            verts.push_back(vert);
            grid.push_back(index++);
        }
    }

    const size_t rowSize = (size_t)params->widthSeg + 1;
    for (size_t iy = 0; iy < params->heightSeg; iy++) {
        for (size_t ix = 0; ix < params->widthSeg; ix++) {

            const u32 a = grid[(iy * rowSize) + ix + 1];
            const u32 b = grid[(iy * rowSize) + ix];
            const u32 c = grid[(rowSize * (iy + 1)) + ix];
            const u32 d = grid[rowSize * (iy + 1) + (ix + 1)];

            if (iy != 0 || params->thetaStart > EPSILON) {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(d);
            }
            if (iy != (size_t)params->heightSeg - 1 || thetaEnd < PI - EPSILON) {
                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(d);
            }
        }
    }

    Vertices::copy(vertices, verts.data(), (u32)verts.size(), indices.data(),
                   (u32)indices.size());
}

// ============================================================================
// Arena builders
// ============================================================================

struct gvec3i {
    u32 x, y, z;
};

struct gvec2f {
    f32 x, y;
};

struct gvec3f {
    f32 x, y, z;
};

struct gvec4f {
    f32 x, y, z, w;
};

static int GAB_indicesCount(GeometryArenaBuilder* builder)
{
    return ARENA_LENGTH(builder->indices_arena, u32);
}

static int GAB_vertexCount(GeometryArenaBuilder* builder)
{
    return ARENA_LENGTH(builder->pos_arena, gvec3f);
}

static int GAB_faceCount(GeometryArenaBuilder* builder)
{
    int indices_count = GAB_indicesCount(builder);
    int ret = (indices_count > 0) ? indices_count / 3 : GAB_vertexCount(builder) / 3;
    return ret;
}

static int GAB_indexFromFace(GeometryArenaBuilder* builder, int iFace, int iVert)
{
    int indices_count  = GAB_indicesCount(builder);
    u32* indices_array = (u32*)(builder->indices_arena->base);
    return (indices_count > 0) ? indices_array[iFace * 3 + iVert] : (iFace * 3 + iVert);
}

static void Geometry_computeTangents(GeometryArenaBuilder* builder)
{
    SMikkTSpaceInterface mikktspaceIface = {};
    SMikkTSpaceContext mikktspaceContext = {};
    mikktspaceContext.m_pInterface       = &mikktspaceIface;
    mikktspaceContext.m_pUserData        = builder;

    mikktspaceIface.m_getNumFaces = [](const SMikkTSpaceContext* pContext) {
        return GAB_faceCount((GeometryArenaBuilder*)pContext->m_pUserData);
    };

    mikktspaceIface.m_getNumVerticesOfFace
      = [](const SMikkTSpaceContext* pContext, const int iFace) {
            return 3; // triangles only
        };

    mikktspaceIface.m_getPosition = [](const SMikkTSpaceContext* pContext,
                                       f32 fvPosOut[], const int iFace,
                                       const int iVert) {
        GeometryArenaBuilder* builder = (GeometryArenaBuilder*)pContext->m_pUserData;

        int index = GAB_indexFromFace(builder, iFace, iVert);

        f32* positions = (f32*)(builder->pos_arena->base);
        fvPosOut[0]    = positions[index * 3 + 0];
        fvPosOut[1]    = positions[index * 3 + 1];
        fvPosOut[2]    = positions[index * 3 + 2];
    };

    mikktspaceIface.m_getNormal = [](const SMikkTSpaceContext* pContext,
                                     f32 fvNormOut[], const int iFace,
                                     const int iVert) {
        GeometryArenaBuilder* builder = (GeometryArenaBuilder*)pContext->m_pUserData;

        int index = GAB_indexFromFace(builder, iFace, iVert);

        f32* normals = (f32*)(builder->norm_arena->base);
        fvNormOut[0] = normals[index * 3 + 0];
        fvNormOut[1] = normals[index * 3 + 1];
        fvNormOut[2] = normals[index * 3 + 2];
    };

    mikktspaceIface.m_getTexCoord = [](const SMikkTSpaceContext* pContext,
                                       f32 fvTexcOut[], const int iFace,
                                       const int iVert) {
        GeometryArenaBuilder* builder = (GeometryArenaBuilder*)pContext->m_pUserData;

        int index = GAB_indexFromFace(builder, iFace, iVert);

        f32* texcoords = (f32*)(builder->uv_arena->base);
        fvTexcOut[0]   = texcoords[index * 2 + 0];
        fvTexcOut[1]   = texcoords[index * 2 + 1];
    };

    mikktspaceIface.m_setTSpaceBasic = [](const SMikkTSpaceContext* pContext,
                                          const f32 fvTangent[], const f32 fSign,
                                          const int iFace, const int iVert) {
        GeometryArenaBuilder* builder = (GeometryArenaBuilder*)pContext->m_pUserData;

        int index = GAB_indexFromFace(builder, iFace, iVert);

        gvec4f* tangents = (gvec4f*)(builder->tangent_arena->base);
        // make sure index within arena bounds
        ASSERT(index < ARENA_LENGTH(builder->tangent_arena, gvec4f));
        tangents[index] = { fvTangent[0], fvTangent[1], fvTangent[2], fSign };
    };

    genTangSpaceDefault(&mikktspaceContext);
}

void Geometry_buildPlane(GeometryArenaBuilder* builder, PlaneParams* params)
{
    const f32 width_half  = params->width * 0.5f;
    const f32 height_half = params->height * 0.5f;

    const u32 gridX = params->widthSegments;
    const u32 gridY = params->heightSegments;

    const u32 gridX1 = gridX + 1;
    const u32 gridY1 = gridY + 1;

    const f32 segment_width  = params->width / gridX;
    const f32 segment_height = params->height / gridY;

    const u32 vertex_count    = gridX1 * gridY1;
    const u32 index_tri_count = gridX * gridY * 2;

    // initialize arena memory
    gvec3f* pos_array  = ARENA_PUSH_COUNT(builder->pos_arena, gvec3f, vertex_count);
    gvec3f* norm_array = ARENA_PUSH_COUNT(builder->norm_arena, gvec3f, vertex_count);
    gvec2f* uv_array   = ARENA_PUSH_COUNT(builder->uv_arena, gvec2f, vertex_count);
    gvec4f* tangent_array
      = ARENA_PUSH_COUNT(builder->tangent_arena, gvec4f, vertex_count);
    gvec3i* indices_array
      = ARENA_PUSH_COUNT(builder->indices_arena, gvec3i, index_tri_count);

    u32 index = 0;
    for (u32 iy = 0; iy < gridY1; iy++) {
        const f32 y = iy * segment_height - height_half;
        for (u32 ix = 0; ix < gridX1; ix++) {
            const f32 x       = ix * segment_width - width_half;
            pos_array[index]  = { x, -y, 0 };
            norm_array[index] = { 0, 0, 1 };
            uv_array[index]   = { (f32)ix / gridX, 1.0f - ((f32)iy / gridY) };

            ++index;
        }
    }
    ASSERT(index == vertex_count);

    index = 0;
    for (u32 iy = 0; iy < gridY; iy++) {
        for (u32 ix = 0; ix < gridX; ix++) {
            const u32 a = ix + gridX1 * iy;
            const u32 b = ix + gridX1 * (iy + 1);
            const u32 c = (ix + 1) + gridX1 * (iy + 1);
            const u32 d = (ix + 1) + gridX1 * iy;

            indices_array[index++] = { a, b, d };
            indices_array[index++] = { b, c, d };
        }
    }
    ASSERT(index == index_tri_count);

    // build tangents
    Geometry_computeTangents(builder);
    ASSERT(ARENA_LENGTH(builder->tangent_arena, gvec4f) == vertex_count);
}

void Geometry_buildSphere(GeometryArenaBuilder* builder, SphereParams* params)
{

    params->widthSeg  = MAX(3, params->widthSeg);
    params->heightSeg = MAX(2, params->heightSeg);

    const f32 thetaEnd = MIN(params->thetaStart + params->thetaLength, PI);

    u32 index = 0;

    std::vector<u32> grid;
    std::vector<Vertex> verts;
    std::vector<u32> indices;

    // generate vertices, normals and uvs
    for (u32 iy = 0; iy <= params->heightSeg; iy++) {

        const f32 v = (f32)iy / (f32)params->heightSeg;

        // special case for the poles
        f32 uOffset = 0;
        if (iy == 0 && glm::epsilonEqual(params->thetaStart, 0.0f, EPSILON)) {
            uOffset = 0.5f / params->widthSeg;
        } else if (iy == params->heightSeg
                   && glm::epsilonEqual(thetaEnd, PI, EPSILON)) {
            uOffset = -0.5 / params->widthSeg;
        }

        for (u32 ix = 0; ix <= params->widthSeg; ix++) {

            const f32 u = (f32)ix / (f32)params->widthSeg;

            Vertex vert;

            // vertex
            vert.x = -params->radius
                     * glm::cos(params->phiStart + u * params->phiLength)
                     * glm::sin(params->thetaStart + v * params->thetaLength);
            vert.y
              = params->radius * glm::cos(params->thetaStart + v * params->thetaLength);
            vert.z = params->radius * glm::sin(params->phiStart + u * params->phiLength)
                     * glm::sin(params->thetaStart + v * params->thetaLength);

            // normal
            glm::vec3 normal = glm::normalize(glm::vec3(vert.x, vert.y, vert.z));
            vert.nx          = normal.x;
            vert.ny          = normal.y;
            vert.nz          = normal.z;

            // uv
            vert.u = u + uOffset;
            vert.v = 1 - v;

            verts.push_back(vert);
            grid.push_back(index++);
        }
    }

    const size_t rowSize = (size_t)params->widthSeg + 1;
    for (size_t iy = 0; iy < params->heightSeg; iy++) {
        for (size_t ix = 0; ix < params->widthSeg; ix++) {

            const u32 a = grid[(iy * rowSize) + ix + 1];
            const u32 b = grid[(iy * rowSize) + ix];
            const u32 c = grid[(rowSize * (iy + 1)) + ix];
            const u32 d = grid[rowSize * (iy + 1) + (ix + 1)];

            if (iy != 0 || params->thetaStart > EPSILON) {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(d);
            }
            if (iy != (size_t)params->heightSeg - 1 || thetaEnd < PI - EPSILON) {
                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(d);
            }
        }
    }

    size_t vertex_count = verts.size();
    size_t index_count  = indices.size();
    ASSERT(index_count % 3 == 0); // must be a multiple of 3 triangles only

    gvec3f* positions  = ARENA_PUSH_COUNT(builder->pos_arena, gvec3f, vertex_count);
    gvec3f* normals    = ARENA_PUSH_COUNT(builder->norm_arena, gvec3f, vertex_count);
    gvec2f* texcoords  = ARENA_PUSH_COUNT(builder->uv_arena, gvec2f, vertex_count);
    gvec4f* tangents   = ARENA_PUSH_COUNT(builder->tangent_arena, gvec4f, vertex_count);
    u32* indices_array = ARENA_PUSH_COUNT(builder->indices_arena, u32, index_count);

    for (u32 i = 0; i < vertex_count; i++) {
        positions[i] = { verts[i].x, verts[i].y, verts[i].z };
        normals[i]   = { verts[i].nx, verts[i].ny, verts[i].nz };
        texcoords[i] = { verts[i].u, verts[i].v };
    }

    memcpy(indices_array, indices.data(), indices.size() * sizeof(*indices_array));
    ASSERT(ARENA_LENGTH(builder->indices_arena, u32) == index_count);

    // build tangents
    Geometry_computeTangents(builder);
    ASSERT(ARENA_LENGTH(builder->tangent_arena, gvec4f) == vertex_count);
}
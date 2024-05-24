#include "geometry.h"

#include "core/memory.h"

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
        v->vertexData
          = ALLOCATE_COUNT(f32, vertexCount * CHUGL_FLOATS_PER_VERTEX);
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
        printf("\t%d %d %d\n", v->indices[i], v->indices[i + 1],
               v->indices[i + 2]);
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
        return (int)(v->indicesCount > 0 ? v->indicesCount / 3 :
                                           v->vertexCount / 3);
    };

    mikktspaceIface.m_getNumVerticesOfFace
      = [](const SMikkTSpaceContext* pContext, const int iFace) {
            return 3; // triangles only
        };

    mikktspaceIface.m_getPosition
      = [](const SMikkTSpaceContext* pContext, float fvPosOut[],
           const int iFace, const int iVert) {
            Vertices* v = (Vertices*)pContext->m_pUserData;
            u32 index = (v->indicesCount > 0) ? v->indices[iFace * 3 + iVert] :
                                                (iFace * 3 + iVert);
            f32* positions = Vertices::positions(v);
            fvPosOut[0]    = positions[index * 3 + 0];
            fvPosOut[1]    = positions[index * 3 + 1];
            fvPosOut[2]    = positions[index * 3 + 2];
        };

    mikktspaceIface.m_getNormal
      = [](const SMikkTSpaceContext* pContext, float fvNormOut[],
           const int iFace, const int iVert) {
            Vertices* v = (Vertices*)pContext->m_pUserData;
            u32 index = (v->indicesCount > 0) ? v->indices[iFace * 3 + iVert] :
                                                (iFace * 3 + iVert);
            f32* normals = Vertices::normals(v);
            fvNormOut[0] = normals[index * 3 + 0];
            fvNormOut[1] = normals[index * 3 + 1];
            fvNormOut[2] = normals[index * 3 + 2];
        };

    mikktspaceIface.m_getTexCoord
      = [](const SMikkTSpaceContext* pContext, float fvTexcOut[],
           const int iFace, const int iVert) {
            Vertices* v = (Vertices*)pContext->m_pUserData;
            u32 index = (v->indicesCount > 0) ? v->indices[iFace * 3 + iVert] :
                                                (iFace * 3 + iVert);
            f32* texcoords = Vertices::texcoords(v);
            fvTexcOut[0]   = texcoords[index * 2 + 0];
            fvTexcOut[1]   = texcoords[index * 2 + 1];
        };

    mikktspaceIface.m_setTSpaceBasic
      = [](const SMikkTSpaceContext* pContext, const float fvTangent[],
           const float fSign, const int iFace, const int iVert) {
            Vertices* v = (Vertices*)pContext->m_pUserData;
            u32 index = (v->indicesCount > 0) ? v->indices[iFace * 3 + iVert] :
                                                (iFace * 3 + iVert);
            f32* tangents           = Vertices::tangents(v);
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

void Vertices::copy(Vertices* v, Vertex* vertices, u32 vertexCount,
                    u32* indices, u32 indicesCount)
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
Vertices createPlane(const PlaneParams* params)
{
    Vertices vertices     = {};
    const f32 width_half  = params->width * 0.5f;
    const f32 height_half = params->height * 0.5f;

    const u32 gridX = params->widthSegments;
    const u32 gridY = params->heightSegments;

    const u32 gridX1 = gridX + 1;
    const u32 gridY1 = gridY + 1;

    const f32 segment_width  = params->width / gridX;
    const f32 segment_height = params->height / gridY;

    Vertices::init(&vertices, gridX1 * gridY1, gridX * gridY * 6);

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

            Vertices::setVertex(&vertices, vert, index++);
        }
    }
    ASSERT(index == vertices.vertexCount);
    index = 0;
    for (u32 iy = 0; iy < gridY; iy++) {
        for (u32 ix = 0; ix < gridX; ix++) {
            const u32 a = ix + gridX1 * iy;
            const u32 b = ix + gridX1 * (iy + 1);
            const u32 c = (ix + 1) + gridX1 * (iy + 1);
            const u32 d = (ix + 1) + gridX1 * iy;

            Vertices::setIndices(&vertices, a, b, d, index++);
            Vertices::setIndices(&vertices, b, c, d, index++);
        }
    }
    ASSERT(index == vertices.indicesCount / 3);
    return vertices;
}

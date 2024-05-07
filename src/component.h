#pragma once

#include "graphics.h"

#include "core/macros.h"
#include "core/memory.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>

#define QUAT_IDENTITY (glm::quat(1.0, 0.0, 0.0, 0.0))
#define MAT_IDENTITY (glm::mat4(1.0))

#define VEC_ORIGIN (glm::vec3(0.0f, 0.0f, 0.0f))
#define VEC_UP (glm::vec3(0.0f, 1.0f, 0.0f))
#define VEC_DOWN (glm::vec3(0.0f, -1.0f, 0.0f))
#define VEC_LEFT (glm::vec3(-1.0f, 0.0f, 0.0f))
#define VEC_RIGHT (glm::vec3(1.0f, 0.0f, 0.0f))
#define VEC_FORWARD (glm::vec3(0.0f, 0.0f, -1.0f))
#define VEC_BACKWARD (glm::vec3(0.0f, 0.0f, 1.0f))

// TODO: write readme on architecture

// =============================================================================
// scenegraph data structures
// =============================================================================
// TODO: eventually use uniform Arena allocators for all component types
// and track location via a custom hashmap from ID --> arena offset

struct Vertices;

enum SG_ComponentType {
    SG_COMPONENT_INVALID,
    SG_COMPONENT_TRANSFORM,
    SG_COMPONENT_GEOMETRY,
    SG_COMPONENT_MATERIAL,
    SG_COMPONENT_COUNT
};

typedef u64 SG_ID;

struct SG_Component {
    SG_ID id;
    SG_ComponentType type;
    std::string name;

    // std::string name; ?
};

// priority hiearchy for staleness
enum SG_Transform_Staleness {
    SG_TRANSFORM_STALE_NONE = 0,

    SG_TRANSFORM_STALE_DESCENDENTS, // at least 1 descendent must recompute
                                    // world matrix

    SG_TRANSFORM_STALE_WORLD, // world matrix of self and all descendents must
                              // be recomputed

    SG_TRANSFORM_STALE_LOCAL, // local matrix of self must be recomputed,
                              // AND world matrix of self and all descendents
                              // must be recomputed
    SG_TRANSFORM_STALE_COUNT
};

struct SG_Transform : public SG_Component {
    // staleness flag has priority hiearchy, don't set directly!
    // instead use setStale()
    SG_Transform_Staleness _stale;

    // transform
    // don't update directly, otherwise staleness will be incorrect
    // use pos(), rot(), sca() instead
    glm::vec3 _pos;
    glm::quat _rot;
    glm::vec3 _sca;

    // world matrix (TODO cache)
    glm::mat4 world;
    glm::mat4 local;

    // not tracking parent to prevent cycles
    // TODO: should we store a list of children directly, or pointers to
    // children? storing children directly has better cache performs, but will
    // invalidate pointers to these children on insertion/removal operations
    // storing pointers to children for now, probably replace with child ID
    // later e.g. every entity has ID, and components have unique ID as well?
    SG_ID parentID;
    Arena children; // stores list of SG_IDs

    SG_ID _geoID; // don't modify directly; use SG_Geo::addXform() instead

    static void init(SG_Transform* transform);

    static void setStale(SG_Transform* xform, SG_Transform_Staleness stale);

    static glm::mat4 localMatrix(SG_Transform* xform);

    /// @brief decompose matrix into transform data
    static void setXformFromMatrix(SG_Transform* xform, const glm::mat4& M);

    static void setXform(SG_Transform* xform, const glm::vec3& pos,
                         const glm::quat& rot, const glm::vec3& sca);
    static void pos(SG_Transform* xform, const glm::vec3& pos);
    static void rot(SG_Transform* xform, const glm::quat& rot);
    static void sca(SG_Transform* xform, const glm::vec3& sca);

    // updates all local/world matrices in the scenegraph
    static void rebuildMatrices(SG_Transform* root, Arena* arena);

    // Scenegraph relationships ----------------------------------------------
    // returns if ancestor is somewhere in the parent chain of descendent,
    // including descendent itself
    static bool isAncestor(SG_Transform* ancestor, SG_Transform* descendent);
    static void removeChild(SG_Transform* parent, SG_Transform* child);
    static void addChild(SG_Transform* parent, SG_Transform* child);
    static u32 numChildren(SG_Transform* xform);
    static SG_Transform* getChild(SG_Transform* xform, u32 index);

    // Transform modification ------------------------------------------------
    static void rotateOnLocalAxis(SG_Transform* xform, glm::vec3 axis, f32 deg);
    static void rotateOnWorldAxis(SG_Transform* xform, glm::vec3 axis, f32 deg);

    // util -------------------------------------------------------------------
    static void print(SG_Transform* xform, u32 depth = 0);
};

struct SG_Geometry : public SG_Component {

    IndexBuffer indexBuffer;
    VertexBuffer vertexBuffer;
    u32 numVertices;
    u32 numIndices;

    // associated xform instances
    Arena xformIDs;

    // bindgroup
    WGPUBindGroupEntry bindGroupEntry;
    WGPUBindGroup bindGroup;
    WGPUBuffer storageBuffer;
    u32 storageBufferCap; // capacity in number of FrameUniforms NOT bytes
    bool staleBindGroup;  // true if storage buffer needs to be rebuilt with new
                          // world matrices

    static void init(SG_Geometry* geo);
    static void buildFromVertices(GraphicsContext* gctx, SG_Geometry* geo,
                                  Vertices* vertices);
    static u64 numInstances(SG_Geometry* geo);

    // functions for adding/removing instances
    static void addXform(SG_Geometry* geo, SG_Transform* xform);
    static void removeXform(SG_Geometry* geo, SG_Transform* xform);

    // uploads xform data to storage buffer
    static void rebuildBindGroup(GraphicsContext* gctx, SG_Geometry* geo,
                                 WGPUBindGroupLayout layout, Arena* arena);
};

// =============================================================================
// Component Manager API
// =============================================================================

SG_Transform* Component_CreateTransform();
SG_Geometry* Component_CreateGeometry();

SG_Transform* Component_GetXform(u64 id);
SG_Geometry* Component_GetGeo(u64 id);

void Component_Init();
void Component_Free();

// TODO: add destroy functions. Remember to change offsets after swapping!
// should these live in the components?
// void Component_DestroyXform(u64 id);
/*
Enforcing pointer safety:
- hide all component initialization fns as static within component.cpp
    - only the manager can create/delete components
    - similar to how all memory allocations are routed through realloc
- all component accesses happen via IDs routed through the manager
    - IDs, unlike pointers, are safe to store
    - if the component created by that ID is deleted, the ID lookup will yield
    NULL, and the calling code will likely crash with a NULL pointer dereference
    (easy to debug)
- all deletions / GC are deferred to the VERY END of the frame
    - prevents bug where a component is deleted WHILE it is being used after an
    ID lookup
    - enforce hygiene of never storing / carrying pointers across frame
    boundaries (within is ok)
    - also enables a more controllable GC system
*/
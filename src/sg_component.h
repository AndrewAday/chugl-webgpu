#pragma once

#include "graphics.h"

#include "core/macros.h"
#include "core/memory.h"

#include <chuck/chugin.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream> // std::string

#define QUAT_IDENTITY (glm::quat(1.0, 0.0, 0.0, 0.0))
#define MAT_IDENTITY (glm::mat4(1.0))

#define VEC_ORIGIN (glm::vec3(0.0f, 0.0f, 0.0f))
#define VEC_UP (glm::vec3(0.0f, 1.0f, 0.0f))
#define VEC_DOWN (glm::vec3(0.0f, -1.0f, 0.0f))
#define VEC_LEFT (glm::vec3(-1.0f, 0.0f, 0.0f))
#define VEC_RIGHT (glm::vec3(1.0f, 0.0f, 0.0f))
#define VEC_FORWARD (glm::vec3(0.0f, 0.0f, -1.0f))
#define VEC_BACKWARD (glm::vec3(0.0f, 0.0f, 1.0f))
#define VEC_ONES (glm::vec3(1.0f, 1.0f, 1.0f))

typedef t_CKUINT SG_ID;

// (enum, ckname)
#define SG_ComponentTable                                                      \
    X(SG_COMPONENT_INVALID = 0, "Invalid")                                     \
    X(SG_COMPONENT_BASE, "SG_Component")                                       \
    X(SG_COMPONENT_TRANSFORM, "GGen")                                          \
    X(SG_COMPONENT_SCENE, "GScene")                                            \
    X(SG_COMPONENT_GEOMETRY, "Geometry")                                       \
    X(SG_COMPONENT_MATERIAL, "Material")                                       \
    X(SG_COMPONENT_TEXTURE, "Texture")                                         \
    X(SG_COMPONENT_MESH, "GMesh")

enum SG_ComponentType {
#define X(name, str) name,
    SG_ComponentTable
#undef X
      SG_COMPONENT_COUNT
};

static const char* SG_CKNames[SG_COMPONENT_COUNT] = {
#define X(name, str) str,
    SG_ComponentTable
#undef X
};

struct SG_Component {
    SG_ID id;
    SG_ComponentType type;
    std::string name; // TODO move off std::string
    Chuck_Object* ckobj;
    // TODO cache hash
    // u64 hash;
};

// ============================================================================
// SG_Sampler
// ============================================================================

enum SG_Sampler_WrapMode : u8 {
    SG_SAMPLER_WRAP_REPEAT        = 0,
    SG_SAMPLER_WRAP_MIRROR_REPEAT = 1,
    SG_SAMPLER_WRAP_CLAMP_TO_EDGE = 2,
};

enum SG_Sampler_FilterMode : u8 {
    SG_SAMPLER_FILTER_NEAREST = 0,
    SG_SAMPLER_FILTER_LINEAR  = 1,
};

struct SG_Sampler {
    SG_Sampler_WrapMode wrapU, wrapV, wrapW;
    SG_Sampler_FilterMode filterMin, filterMag, filterMip;
};

static SG_Sampler SG_SAMPLER_DEFAULT // make this a #define instead?
  = { SG_SAMPLER_WRAP_REPEAT,   SG_SAMPLER_WRAP_REPEAT,
      SG_SAMPLER_WRAP_REPEAT,   SG_SAMPLER_FILTER_LINEAR,
      SG_SAMPLER_FILTER_LINEAR, SG_SAMPLER_FILTER_LINEAR };

// ============================================================================
// SG_Texture
// ============================================================================
struct SG_Texture : SG_Component {
};

// ============================================================================
// SG_Transform
// ============================================================================

struct SG_Transform : public SG_Component {
    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 sca;

    // relationships
    SG_ID parentID;
    Arena childrenIDs;

    // TODO: come up with staleness scheme that makes sense for scenegraph

    // don't init directly. Use SG Component Manager instead
    static void _init(SG_Transform* t, Chuck_Object* ckobj);

    static void translate(SG_Transform* t, glm::vec3 delta);
    static void rotate(SG_Transform* t, glm::quat q);
    static void rotate(SG_Transform* t, glm::vec3 eulers);
    static void scale(SG_Transform* t, glm::vec3 s);

    static void rotateOnWorldAxis(SG_Transform* t, glm::vec3 axis, float rad);
    static void rotateOnLocalAxis(SG_Transform* t, glm::vec3 axis, float rad);
    static void rotateX(SG_Transform* t, float deg);
    static void rotateY(SG_Transform* t, float deg);
    static void rotateZ(SG_Transform* t, float deg);
    static void lookAt(SG_Transform* t, glm::vec3 pos);

    static glm::vec3 eulerRotationRadians(SG_Transform* t);
    static glm::mat4 modelMatrix(SG_Transform* t);
    static glm::mat4 worldMatrix(SG_Transform* t);
    static glm::quat worldRotation(SG_Transform* t);
    static glm::vec3 worldPosition(SG_Transform* t);
    static glm::vec3 worldScale(SG_Transform* t);
    static void worldPosition(SG_Transform* t, glm::vec3 pos);
    static void worldScale(SG_Transform* t, glm::vec3 scale);
    static glm::vec3 right(SG_Transform* t);
    static glm::vec3 forward(SG_Transform* t);
    static glm::vec3 up(SG_Transform* t);

    // SceneGraph relationships ========================================
    static void addChild(SG_Transform* parent, SG_Transform* child);
    static void removeChild(SG_Transform* parent, SG_Transform* child);
    static bool isAncestor(SG_Transform* ancestor, SG_Transform* descendent);
    static size_t numChildren(SG_Transform* t);
    static SG_Transform* child(SG_Transform* t, size_t index);

    // disconnect from both parent and children
    // void Disconnect( bool sendChildrenToGrandparent = false );
};

// ============================================================================
// SG_Scene
// ============================================================================

struct SG_Scene : public SG_Transform {
    glm::vec4 bg_color;
};

// ============================================================================
// SG_Geometry
// ============================================================================

enum SG_GeometryType : u8 {
    SG_GEOMETRY_INVALID = 0,
    SG_GEOMETRY_PLANE,
    SG_GEOMETRY_CUBE,
    SG_GEOMETRY_SPHERE,
    SG_GEOMETRY_CYLINDER,
    SG_GEOMETRY_CONE,
    SG_GEOMETRY_TORUS,
    SG_GEOMETRY_CUSTOM,
    SG_GEOMETRY_COUNT
};

union SG_GeometryParams {
    PlaneParams plane;
    SphereParams sphere;
};

// immutable
struct SG_Geometry : SG_Component {
    SG_GeometryType geo_type;
    // TODO add cpu-side vertex data
    SG_GeometryParams params;

    /// @brief
    /// @param params pointer to struct containing geometry parameters
    static void _init(SG_Geometry* g, SG_GeometryType geo_type, void* params);
};

// ============================================================================
// SG_Material
// ============================================================================

enum SG_MaterialType : u8 {
    SG_MATERIAL_INVALID = 0,
    SG_MATERIAL_PBR,
    SG_MATERIAL_CUSTOM,
    SG_MATERIAL_COUNT
};

struct SG_Material_PBR_Params {
    // uniforms
    glm::vec4 baseColor      = glm::vec4(1.0f);
    glm::vec3 emissiveFactor = glm::vec3(0.0f);
    f32 metallic             = 0.0f;
    f32 roughness            = 1.0f;
    f32 normalFactor         = 1.0f;
    f32 aoFactor             = 1.0f;

    // textures and samplers
    // TODO
    // SG_Sampler baseColorSampler;
};

// TODO if discrepency between material params too large,
// switch to allocated void* rather than union
// Then SG_Command_MaterialCreate will need a pointer too...
union SG_MaterialParams {
    SG_Material_PBR_Params pbr;
};

struct SG_Material : SG_Component {
    SG_MaterialType material_type;
    SG_MaterialParams params;
};

// ============================================================================
// SG_Mesh
// ============================================================================

// in SG we'll try deep: SG_Mesh inherits SG_Transform
// in R we'll try flat: R_Transform contains union { mesh, light, camera }
struct SG_Mesh : SG_Transform {
    // don't set directly. use setGeometry and setMaterial for proper
    // refcounting
    SG_ID _geo_id;
    SG_ID _mat_id;

    static void setGeometry(SG_Mesh* mesh, SG_Geometry* geo);
    static void setMaterial(SG_Mesh* mesh, SG_Material* mat);
};

// ============================================================================
// SG Component Manager
// ============================================================================

void SG_Init(const Chuck_DL_Api* api);
void SG_Free();

SG_Transform* SG_CreateTransform(Chuck_Object* ckobj);
SG_Scene* SG_CreateScene(Chuck_Object* ckobj);
SG_Geometry* SG_CreateGeometry(Chuck_Object* ckobj, SG_GeometryType geo_type,
                               void* params);
SG_Material* SG_CreateMaterial(Chuck_Object* ckobj,
                               SG_MaterialType material_type, void* params);
SG_Mesh* SG_CreateMesh(Chuck_Object* ckobj, SG_Geometry* sg_geo,
                       SG_Material* sg_mat);

SG_Component* SG_GetComponent(SG_ID id);
SG_Transform* SG_GetTransform(SG_ID id);
SG_Scene* SG_GetScene(SG_ID id);
SG_Geometry* SG_GetGeometry(SG_ID id);
SG_Material* SG_GetMaterial(SG_ID id);
SG_Mesh* SG_GetMesh(SG_ID id);

// ============================================================================
// SG Garbage Collection
// ============================================================================

void SG_DecrementRef(SG_ID id);
void SG_AddRef(SG_Component* comp);
void SG_GC();
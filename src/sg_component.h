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
#define SG_ComponentTable                                                              \
    X(SG_COMPONENT_INVALID = 0, "Invalid")                                             \
    X(SG_COMPONENT_BASE, "SG_Component")                                               \
    X(SG_COMPONENT_TRANSFORM, "GGen")                                                  \
    X(SG_COMPONENT_SCENE, "GScene")                                                    \
    X(SG_COMPONENT_GEOMETRY, "Geometry")                                               \
    X(SG_COMPONENT_SHADER, "Shader")                                                   \
    X(SG_COMPONENT_MATERIAL, "Material")                                               \
    X(SG_COMPONENT_TEXTURE, "Texture")                                                 \
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
    char name[64] = {};
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
  = { SG_SAMPLER_WRAP_REPEAT,   SG_SAMPLER_WRAP_REPEAT,   SG_SAMPLER_WRAP_REPEAT,
      SG_SAMPLER_FILTER_LINEAR, SG_SAMPLER_FILTER_LINEAR, SG_SAMPLER_FILTER_LINEAR };

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
    SG_GEOMETRY = 0, // base class. doubles as custom geometry
    SG_GEOMETRY_PLANE,
    SG_GEOMETRY_CUBE,
    SG_GEOMETRY_SPHERE,
    SG_GEOMETRY_CYLINDER,
    SG_GEOMETRY_CONE,
    SG_GEOMETRY_TORUS,
    SG_GEOMETRY_COUNT
};

union SG_GeometryParams {
    PlaneParams plane;
    SphereParams sphere;
};

#define SG_GEOMETRY_MAX_VERTEX_ATTRIBUTES 8

#define SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION 0
#define SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION 1
#define SG_GEOMETRY_UV_ATTRIBUTE_LOCATION 2
#define SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION 3

struct SG_Geometry : SG_Component {
    SG_GeometryType geo_type;
    SG_GeometryParams params;

    // buffers to hold vertex data
    Arena vertex_attribute_data[SG_GEOMETRY_MAX_VERTEX_ATTRIBUTES];
    int vertex_attribute_num_components[SG_GEOMETRY_MAX_VERTEX_ATTRIBUTES];
    Arena indices;

    static u32 vertexCount(SG_Geometry* geo);
    static u32 indexCount(SG_Geometry* geo);

    // data_len is length of data in floats, not bytes not components
    static f32* setAttribute(SG_Geometry* geo, int location, int num_components,
                             CK_DL_API api, Chuck_ArrayFloat* ck_array, int ck_arr_len);
    static u32* setIndices(SG_Geometry* geo, CK_DL_API API, Chuck_ArrayInt* indices,
                           int index_count);
    static u32* getIndices(SG_Geometry* geo);

    static f32* getAttributeData(SG_Geometry* geo, int location);

    // builder functions
    static void buildPlane(SG_Geometry* g, PlaneParams* p);
    static void buildSphere(SG_Geometry* g, SphereParams* p);
};

// ============================================================================
// SG_Shader
// ============================================================================

struct SG_Shader : SG_Component {
    const char* vertex_string_owned;
    const char* fragment_string_owned;
    const char* vertex_filepath_owned;
    const char* fragment_filepath_owned;
    int vertex_layout[SG_GEOMETRY_MAX_VERTEX_ATTRIBUTES];
};

// ============================================================================
// SG_Material
// ============================================================================

enum SG_MaterialType : u8 {
    SG_MATERIAL_INVALID = 0,
    SG_MATERIAL_CUSTOM,
    SG_MATERIAL_PBR,
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

#define SG_MATERIAL_MAX_UNIFORMS 32 // @group(1) @binding(0 - 31)

enum SG_MaterialUniformType : u8 {
    SG_MATERIAL_UNIFORM_NONE = 0,
    SG_MATERIAL_UNIFORM_FLOAT,
    SG_MATERIAL_UNIFORM_VEC2F,
    SG_MATERIAL_UNIFORM_VEC3F,
    SG_MATERIAL_UNIFORM_VEC4F,
    SG_MATERIAL_UNIFORM_INT,
    SG_MATERIAL_UNIFORM_IVEC2,
    SG_MATERIAL_UNIFORM_IVEC3,
    SG_MATERIAL_UNIFORM_IVEC4,
    SG_MATERIAL_UNIFORM_TEXTURE,
    SG_MATERIAL_UNIFORM_SAMPLER,
    SG_MATERIAL_UNIFORM_STORAGE_BUFFER,
};

union SG_MaterialUniformData {
    f32 f;
    glm::vec2 vec2f;
    glm::vec3 vec3f;
    glm::vec4 vec4f;
    i32 i;
    glm::ivec2 ivec2;
    glm::ivec3 ivec3;
    glm::ivec4 ivec4;
};

struct SG_MaterialUniformPtrAndSize {
    void* ptr;
    size_t size;
};

struct SG_MaterialUniform {
    SG_MaterialUniformType type;
    SG_MaterialUniformData as;
    // TODO: texture, sampler, array, storage buffer (array<int> and array<float>)
    // for array storage, can we just store a ref to the ck array itself? avoids
    // duplication

    static SG_MaterialUniformPtrAndSize data(SG_MaterialUniform* u)
    {
        // clang-format off
        switch (u->type) {
            case SG_MATERIAL_UNIFORM_FLOAT: return {&u->as.f, sizeof(u->as.f)};
            case SG_MATERIAL_UNIFORM_VEC2F: return {&u->as.vec2f, sizeof(u->as.vec2f)};
            case SG_MATERIAL_UNIFORM_VEC3F: return {&u->as.vec3f, sizeof(u->as.vec3f)};
            case SG_MATERIAL_UNIFORM_VEC4F: return {&u->as.vec4f, sizeof(u->as.vec4f)};
            case SG_MATERIAL_UNIFORM_INT: return {&u->as.i, sizeof(u->as.i)};
            case SG_MATERIAL_UNIFORM_IVEC2: return {&u->as.ivec2, sizeof(u->as.ivec2)};
            case SG_MATERIAL_UNIFORM_IVEC3: return {&u->as.ivec3, sizeof(u->as.ivec3)};
            case SG_MATERIAL_UNIFORM_IVEC4: return {&u->as.ivec4, sizeof(u->as.ivec4)};
            default: ASSERT(false);
        }
        // clang-format on
        return {};
    }
};

// TODO if discrepency between material params too large,
// switch to allocated void* rather than union
// Then SG_Command_MaterialCreate will need a pointer too...
union SG_MaterialParams {
    SG_Material_PBR_Params pbr;
};

struct SG_MaterialPipelineState {
    SG_ID sg_shader_id;
    WGPUCullMode cull_mode;
};

struct SG_Material : SG_Component {
    SG_MaterialType material_type;
    SG_MaterialParams params;

    // PSO
    SG_MaterialPipelineState pso; // keep it copy by value

    // uniforms
    SG_MaterialUniform uniforms[SG_MATERIAL_MAX_UNIFORMS];

    // fns
    static void removeUniform(SG_Material* mat, int location);

    static void uniformFloat(SG_Material* mat, int location, f32 f);
    // static void uniform(SG_Material* mat, int location, glm::vec2 f2);
    // static void uniform(SG_Material* mat, int location, glm::vec3 f3);
    // static void uniform(SG_Material* mat, int location, glm::vec4 f4);
    // static void uniform(SG_Material* mat, int location, i32 i);
    // static void uniform(SG_Material* mat, int location, glm::ivec2 i2);
    // static void uniform(SG_Material* mat, int location, glm::ivec3 i3);
    // static void uniform(SG_Material* mat, int location, glm::ivec4 i4);
    static f32 uniformFloat(SG_Material* mat, int location);

    static void shader(SG_Material* mat, SG_Shader* shader);
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
SG_Geometry* SG_CreateGeometry(Chuck_Object* ckobj);

// SG_Shader* SG_CreateShader(Chuck_Object* ckobj, const char* vertex_string,
//                              const char* fragment_string, const char*
//                              vertex_filepath, const char* fragment_filepath,
//                              Chuck_ArrayInt* ck_vertex_layout);
SG_Shader* SG_CreateShader(Chuck_Object* ckobj, Chuck_String* vertex_string,
                           Chuck_String* fragment_string, Chuck_String* vertex_filepath,
                           Chuck_String* fragment_filepath,
                           Chuck_ArrayInt* ck_vertex_layout);

SG_Material* SG_CreateMaterial(Chuck_Object* ckobj, SG_MaterialType material_type,
                               void* params);
SG_Mesh* SG_CreateMesh(Chuck_Object* ckobj, SG_Geometry* sg_geo, SG_Material* sg_mat);

SG_Component* SG_GetComponent(SG_ID id);
SG_Transform* SG_GetTransform(SG_ID id);
SG_Scene* SG_GetScene(SG_ID id);
SG_Geometry* SG_GetGeometry(SG_ID id);
SG_Shader* SG_GetShader(SG_ID id);
SG_Material* SG_GetMaterial(SG_ID id);
SG_Mesh* SG_GetMesh(SG_ID id);

// ============================================================================
// SG Garbage Collection
// ============================================================================

void SG_DecrementRef(SG_ID id);
void SG_AddRef(SG_Component* comp);
void SG_GC();
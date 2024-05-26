#pragma once

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

typedef t_CKUINT SG_ID;

#define SG_ComponentTable                                                      \
    X(SG_COMPONENT_INVALID = 0, "Invalid")                                     \
    X(SG_COMPONENT_BASE, "SG_Component")                                       \
    X(SG_COMPONENT_TRANSFORM, "GGen")                                          \
    X(SG_COMPONENT_GEOMETRY, "Geometry")                                       \
    X(SG_COMPONENT_MATERIAL, "Material")                                       \
    X(SG_COMPONENT_TEXTURE, "Texture")

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
    // TODO cache hash
    // u64 hash;

    static const char* ckname(SG_ComponentType type);
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
    static void _init(SG_Transform* t);

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
    // void AddChild(SceneGraphObject* child);
    // void RemoveChild(SceneGraphObject* child);
    // bool HasChild(SceneGraphObject* child);
    // bool BelongsToSceneObject(SceneGraphObject* sgo);

    // disconnect from both parent and children
    // void Disconnect( bool sendChildrenToGrandparent = false );
};

// ============================================================================
// SG Component Manager
// ============================================================================

void SG_Init();
void SG_Free();

// SG_Transform* SG_CreateTransform(Chuck_Object* ckobj, t_CKUINT);
SG_Transform* SG_CreateTransform();

SG_Component* SG_GetComponent(SG_ID id);
SG_Transform* SG_GetTransform(SG_ID id);

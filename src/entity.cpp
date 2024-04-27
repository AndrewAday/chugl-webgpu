#pragma once

#include "geometry.cpp"
#include "graphics.cpp"
#include "shaders.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> // quatToMat4

#define QUAT_IDENTITY (glm::quat(1.0, 0.0, 0.0, 0.0))
#define MAT_IDENTITY (glm::mat4(1.0))

#define VEC_ORIGIN (glm::vec3(0.0f, 0.0f, 0.0f))
#define VEC_UP (glm::vec3(0.0f, 1.0f, 0.0f))
#define VEC_DOWN (glm::vec3(0.0f, -1.0f, 0.0f))
#define VEC_LEFT (glm::vec3(-1.0f, 0.0f, 0.0f))
#define VEC_RIGHT (glm::vec3(1.0f, 0.0f, 0.0f))
#define VEC_FORWARD (glm::vec3(0.0f, 0.0f, -1.0f))
#define VEC_BACKWARD (glm::vec3(0.0f, 0.0f, 1.0f))

enum EntityType {
    ENTITY_TYPE_NONE = 0,
    ENTITY_TYPE_COUNT,
};

struct Entity {
    u64 _id;
    EntityType type;

    // transform
    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 sca;

    // cpu geometry (TODO share across entities)
    Vertices vertices;
    // gpu geometry (renderable) (TODO share across entities)
    VertexBuffer gpuVertices;
    IndexBuffer gpuIndices;

    // BindGroupEntry to hold model uniform buffer
    // currently only model matrices
    BindGroup bindGroup;

    // material (TODO share across entities)

    static void init(Entity* entity, GraphicsContext* ctx,
                     WGPUBindGroupLayout bindGroupLayout);

    static void setVertices(Entity* entity, Vertices* vertices,
                            GraphicsContext* ctx);

    static glm::mat4 modelMatrix(Entity* entity); // TODO cache
    static glm::mat4 viewMatrix(Entity* entity);
    static glm::mat4 projectionMatrix(Entity* entity, f32 aspect);

    static void rotateOnLocalAxis(Entity* entity, glm::vec3 axis, f32 deg);
    static void rotateOnWorldAxis(Entity* entity, glm::vec3 axis, f32 deg);

    static void lookAt(Entity* entity, glm::vec3 target);

    // Idea: just store world matrix and decompose it when needed?
    // TODO: for getting world position, rotation, scale, etc.
    // glm::decompose(matrix, decomposedScale, decomposedRotationQuat,
    // decomposedPosition, skewUnused, perspectiveUnused);
    // https://stackoverflow.com/questions/17918033/glm-decompose-mat4-into-translation-and-rotation
    /* OR
    void decomposeMatrix(const glm::mat4& m, glm::vec3& pos, glm::quat& rot,
    glm::vec3& scale)
    {
        pos = m[3];
        for(int i = 0; i < 3; i++)
            scale[i] = glm::length(vec3(m[i]));

        // PROBLEM: doesn't handle negative scale
        // negate sx if determinant is negative

        const glm::mat3 rotMtx(
            glm::vec3(m[0]) / scale[0],
            glm::vec3(m[1]) / scale[1],
            glm::vec3(m[2]) / scale[2]);
        rot = glm::quat_cast(rotMtx);
    }
    // THREEjs impl here:
    https://github.com/mrdoob/three.js/blob/ef80ac74e6716a50104a57d8add6c8a950bff8d7/src/math/Matrix4.js#L732
    */
};

void Entity::init(Entity* entity, GraphicsContext* ctx,
                  WGPUBindGroupLayout bindGroupLayout)
{
    // zero out
    *entity = {};
    // init transform
    entity->pos = glm::vec3(0.0);
    entity->rot = QUAT_IDENTITY;
    entity->sca = glm::vec3(1.0);

    // init bindgroup for per-entity uniform buffer
    if (bindGroupLayout != NULL) {
        BindGroup::init(ctx, &entity->bindGroup, bindGroupLayout,
                        sizeof(DrawUniforms));
    }
}

// assigns vertices to entity and builds gpu buffers
// immutable: once assigned, vertices cannot be changed
void Entity::setVertices(Entity* entity, Vertices* vertices,
                         GraphicsContext* ctx)
{
    ASSERT(entity->vertices.vertexData == NULL);
    entity->vertices = *vertices; // points to same memory

    // build gpu buffers
    VertexBuffer::init(ctx, &entity->gpuVertices, 8 * vertices->vertexCount,
                       vertices->vertexData, "vertices");
    IndexBuffer::init(ctx, &entity->gpuIndices, vertices->indicesCount,
                      vertices->indices, "indices");
}

glm::mat4 Entity::modelMatrix(Entity* entity)
{
    glm::mat4 M = glm::mat4(1.0);
    M           = glm::translate(M, entity->pos);
    M           = M * glm::toMat4(entity->rot);
    M           = glm::scale(M, entity->sca);
    return M;
}

glm::mat4 Entity::viewMatrix(Entity* entity)
{
    // return glm::inverse(modelMatrix(entity));

    // optimized version for camera only (doesn't take scale into account)
    glm::mat4 invT = glm::translate(MAT_IDENTITY, -entity->pos);
    glm::mat4 invR = glm::toMat4(glm::conjugate(entity->rot));
    return invR * invT;
}

void Entity::rotateOnLocalAxis(Entity* entity, glm::vec3 axis, f32 deg)
{
    entity->rot = entity->rot * glm::angleAxis(deg, glm::normalize(axis));
}

void Entity::rotateOnWorldAxis(Entity* entity, glm::vec3 axis, f32 deg)
{
    entity->rot = glm::angleAxis(deg, glm::normalize(axis)) * entity->rot;
}

void Entity::lookAt(Entity* entity, glm::vec3 target)
{
    // TODO: support scenegraph transform hierarchy for position and rotation
    entity->rot
      = glm::conjugate(glm::toQuat(glm::lookAt(entity->pos, target, VEC_UP)));
}

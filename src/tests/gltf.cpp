#include <cgltf/cgltf.h>
#include <glm/gtc/type_ptr.hpp>

#include "core/log.h"
#include "core/memory.h"
#include "test_base.h"

#include "entity.h"

#include <iostream>

// =============================================================================
// scenegraph data structures
// =============================================================================

struct SG_Transform {
    // transform
    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 sca;

    // world matrix (TODO cache)
    glm::mat4 world;
    glm::mat4 local;

    std::string name;

    // not tracking parent to prevent cycles
    // TODO: should we store a list of children directly, or pointers to
    // children? storing children directly has better cache performs, but will
    // invalidate pointers to these children on insertion/removal operations
    // storing pointers to children for now, probably replace with child ID
    // later e.g. every entity has ID, and components have unique ID as well?
    SG_Transform** children;
    u32 childrenCount;
    u32 childrenCap;

    static void init(SG_Transform* transform)
    {
        transform->pos           = glm::vec3(0.0f);
        transform->rot           = QUAT_IDENTITY;
        transform->sca           = glm::vec3(1.0f);
        transform->world         = MAT_IDENTITY;
        transform->local         = MAT_IDENTITY;
        transform->children      = NULL;
        transform->childrenCount = 0;
        transform->childrenCap   = 0;
        transform->name          = "";
    }

    static void addChild(SG_Transform* parent, SG_Transform* child)
    {
        if (parent->childrenCount >= parent->childrenCap) {
            u32 oldCap          = parent->childrenCap;
            parent->childrenCap = GROW_CAPACITY(oldCap);
            parent->children    = GROW_ARRAY(SG_Transform*, parent->children,
                                             oldCap, parent->childrenCap);
        }
        parent->children[parent->childrenCount++] = child;
    }

    static glm::mat4 modelMatrix(SG_Transform* xform)
    {
        glm::mat4 M = glm::mat4(1.0);
        M           = glm::translate(M, xform->pos);
        M           = M * glm::toMat4(xform->rot);
        M           = glm::scale(M, xform->sca);
        return M;
    }

    /// @brief decompose matrix into transform data
    static void setXformFromMatrix(SG_Transform* xform, const glm::mat4& M)
    {
        xform->pos = glm::vec3(M[3]);
        xform->rot = glm::quat_cast(M);
        xform->sca
          = glm::vec3(glm::length(M[0]), glm::length(M[1]), glm::length(M[2]));
        xform->local = M;
    }

    static void print(SG_Transform* xform, u32 depth = 0)
    {
        for (u32 i = 0; i < depth; ++i) {
            printf("  ");
        }
        printf("%s\n", xform->name.c_str());
        for (u32 i = 0; i < xform->childrenCount; ++i) {
            print(xform->children[i], depth + 1);
        }
    }
};

static GraphicsContext* gctx = NULL;

/// @brief create a vertex buffer from a buffer view.
/// needed because current VertexBuffer::init() doesn't support size in bytes
/// (it should)
/// @param bufferView
/// @return
// static WGPUBuffer vertexBufferFromBufferView(cgltf_buffer_view bufferView)
// {
//     WGPUBufferDescriptor desc = {};
//     desc.size                 = bufferView.size;
//     desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;

//     WGPUBuffer buffer = wgpuDeviceCreateBuffer(gctx->device, &desc);
//     return buffer;
// }

static SG_Transform* gltf_ProcessNode(SG_Transform* parent, cgltf_node* node)
{
    SG_Transform* sg_node = ALLOCATE_TYPE(SG_Transform);
    sg_node->name         = node->name;
    log_trace("processing node: %s", sg_node->name.c_str());

    // xform data
    if (node->has_translation) {
        sg_node->pos = glm::make_vec3(node->translation);
    }
    if (node->has_rotation) {
        sg_node->rot = glm::make_quat(node->rotation);
    }
    if (node->has_scale) {
        sg_node->sca = glm::make_vec3(node->scale);
    }
    if (node->has_matrix) {
        sg_node->local = glm::make_mat4(node->matrix);
    } else {
        sg_node->local = SG_Transform::modelMatrix(sg_node);
    }
    sg_node->world = parent->world * sg_node->local;
    // set xform data if gltf only provided a matrix
    if (!node->has_rotation || !node->has_scale || !node->has_translation)
        SG_Transform::setXformFromMatrix(sg_node, sg_node->local);

    // parent-child relationship
    SG_Transform::addChild(parent, sg_node);

    // process children
    for (cgltf_size i = 0; i < node->children_count; ++i) {
        cgltf_node* child = node->children[i];
        gltf_ProcessNode(sg_node, child);
    }

    return sg_node;

    // process mesh
    // if (node->mesh) {
    //     gltf_ProcessMesh(node->mesh);
    // }
}

static SG_Transform* gltf_ProcessData(cgltf_data* data)
{
    // TODO: use arena allocator for SG nodes?
    SG_Transform* sg_scenes = ALLOCATE_COUNT(SG_Transform, data->scenes_count);

    // process scene -- node transform hierarchy
    for (cgltf_size i = 0; i < data->scenes_count; ++i) {
        cgltf_scene* gltf_scene = &data->scenes[i];
        char* sceneName         = gltf_scene->name;

        SG_Transform* sg_root = sg_scenes + i;
        SG_Transform::init(sg_root);
        sg_root->name = sceneName;

        log_trace("processing scene: %s with %d nodes", sceneName,
                  gltf_scene->nodes_count);

        for (cgltf_size j = 0; j < gltf_scene->nodes_count; ++j) {
            cgltf_node* node = gltf_scene->nodes[j];

            // process node
            gltf_ProcessNode(sg_root, node);
        }
    }
    return sg_scenes;
}

static void _Test_Gltf_OnInit(GraphicsContext* ctx, GLFWwindow* window)
{
    gctx = ctx;
    log_trace("gltf test init");

    cgltf_options options = {};
    // Use our own memory allocation functions
    options.memory.alloc_func
      = [](void* user, cgltf_size size) { return ALLOCATE_BYTES(void, size); };
    options.memory.free_func = [](void* user, void* ptr) { FREE(ptr); };

    cgltf_data* data = NULL;
    cgltf_result result
      = cgltf_parse_file(&options, "./assets/DamagedHelmet.glb", &data);
    if (result == cgltf_result_success) {
        /* TODO make awesome stuff */
        log_trace("gltf parsed successfully");

        SG_Transform* sg_scenes = gltf_ProcessData(data);
        for (u32 i = 0; i < data->scenes_count; ++i) {
            printf("------scene %d------\n", i);
            SG_Transform::print(sg_scenes + i);
        }

        cgltf_free(data);
    }
}

void Test_Gltf(TestCallbacks* callbacks)
{
    *callbacks        = {};
    callbacks->onInit = _Test_Gltf_OnInit;
}
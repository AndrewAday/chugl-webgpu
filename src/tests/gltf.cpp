#include <cgltf/cgltf.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "core/log.h"
#include "core/memory.h"
#include "test_base.h"

#include "entity.h"
#include "geometry.h"
#include "shaders.h"

#include <iostream>
#include <unordered_map>
#include <vector>

static RenderPipeline pipeline = {};
static Material material       = {};
static Texture texture         = {};

// =============================================================================
// scenegraph data structures
// =============================================================================
// TODO: eventually use uniform Arena allocators for all component types
// and track location via a custom hashmap from ID --> arena offset

enum SG_ComponentType {
    SG_COMPONENT_INVALID,
    SG_COMPONENT_TRANSFORM,
    SG_COMPONENT_GEOMETRY,
    SG_COMPONENT_MATERIAL,
    SG_COMPONENT_COUNT
};

struct SG_Component {
    u64 id;
    SG_ComponentType type;
};

static u64 _componentIDCounter = 0;
static u64 getNewComponentID()
{
    return _componentIDCounter++;
}

struct SG_Transform : public SG_Component {
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
        transform->id            = getNewComponentID();
        transform->type          = SG_COMPONENT_TRANSFORM;
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
        log_trace("decomposing matrix");
        xform->pos = glm::vec3(M[3]);
        xform->rot = glm::quat_cast(M);
        xform->sca
          = glm::vec3(glm::length(M[0]), glm::length(M[1]), glm::length(M[2]));
        xform->local = M;

        log_trace("pos: %s", glm::to_string(xform->pos).c_str());
        log_trace("rot: %s", glm::to_string(xform->rot).c_str());
        log_trace("sca: %s", glm::to_string(xform->sca).c_str());
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

struct SG_Geometry : public SG_Component {
    IndexBuffer indexBuffer;
    VertexBuffer vertexBuffer;
    u32 numVertices;
    u32 numIndices;

    // associated xforms
    std::vector<u64> xformIDs; // TODO: use arena allocator

    // bindgroup
    WGPUBindGroupEntry instanceBindGroup;
    WGPUBindGroup bindGroup;
    WGPUBuffer storageBuffer;

    static void init(SG_Geometry* geo)
    {
        geo->id           = getNewComponentID();
        geo->type         = SG_COMPONENT_GEOMETRY;
        geo->indexBuffer  = {};
        geo->vertexBuffer = {};
    }

    static void buildFromVertices(GraphicsContext* gctx, SG_Geometry* geo,
                                  Vertices* vertices)
    {
        IndexBuffer::init(gctx, &geo->indexBuffer, vertices->indicesCount,
                          vertices->indices, NULL);
        const u8 COMPONENTS_PER_VERTEX = 8;
        VertexBuffer::init(gctx, &geo->vertexBuffer,
                           COMPONENTS_PER_VERTEX * vertices->vertexCount,
                           vertices->vertexData, NULL);
        geo->numVertices = vertices->vertexCount;
        geo->numIndices  = vertices->indicesCount;
        log_trace("geo->numVertices: %d", geo->numVertices);
        log_trace("geo->numIndices: %d", geo->numIndices);
    }

    // uploads xform data to storage buffer
    static void rebuildBindGroup(GraphicsContext* gctx, SG_Geometry* geo,
                                 WGPUBindGroupLayout layout)
    {
        // for now hardcoded to only support uniform buffers
        // TODO: support textures, storage buffers, samplers, etc.
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.size             = sizeof(glm::mat4) * geo->xformIDs.size();
        bufferDesc.mappedAtCreation = false;
        // bufferDesc.usage   = WGPUBufferUsage_CopyDst |
        // WGPUBufferUsage_Storage;
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
        // destroy previous
        if (geo->storageBuffer) {
            wgpuBufferDestroy(geo->storageBuffer);
            wgpuBufferRelease(geo->storageBuffer);
        }
        geo->storageBuffer = wgpuDeviceCreateBuffer(gctx->device, &bufferDesc);

        // force only 1 @binding per @group
        WGPUBindGroupEntry binding = {};
        binding.binding            = 0; // @binding(0)
        binding.offset             = 0;
        binding.buffer
          = geo->storageBuffer; // only supports uniform buffers for now
        binding.size = bufferDesc.size;

        // A bind group contains one or multiple bindings
        WGPUBindGroupDescriptor desc = {};
        desc.layout                  = layout;
        desc.entries                 = &binding;
        desc.entryCount              = 1; // force 1 binding per group

        // release previous
        if (geo->bindGroup) wgpuBindGroupRelease(geo->bindGroup);

        geo->bindGroup = wgpuDeviceCreateBindGroup(gctx->device, &desc);
        ASSERT(geo->bindGroup != NULL);
    }
};

struct SG_ComponentManager {
    std::unordered_map<u64, SG_Component*> components;
};

static SG_ComponentManager componentManager;

static SG_Transform* createTransform()
{
    // TODO switch to pool allocators per component type
    SG_Transform* xform = ALLOCATE_TYPE(SG_Transform);
    SG_Transform::init(xform);
    componentManager.components[xform->id] = xform;
    return xform;
}

static SG_Geometry* createGeometry()
{
    SG_Geometry* geo = ALLOCATE_TYPE(SG_Geometry);
    SG_Geometry::init(geo);
    componentManager.components[geo->id] = geo;
    return geo;
}

SG_Component* getComponent(u64 id)
{
    if (componentManager.components.find(id)
        == componentManager.components.end()) {
        log_error("component not found");
        return NULL;
    }
    return componentManager.components[id];
}

#define GET_COMPONENT(type, id) (type*)getComponent(id)

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

static u8 gltf_sizeOfComponentType(cgltf_component_type type)
{
    switch (type) {
        case cgltf_component_type_r_8:
        case cgltf_component_type_r_8u: return 1;
        case cgltf_component_type_r_16:
        case cgltf_component_type_r_16u: return 2;
        case cgltf_component_type_r_32u:
        case cgltf_component_type_r_32f: return 4;
        default: {
            log_error("unknown cgltf component type");
            return 0;
        }
    }
}

static u8 gltf_numComponentsForType(cgltf_type type)
{
    switch (type) {
        case cgltf_type_scalar: return 1;
        case cgltf_type_vec2: return 2;
        case cgltf_type_vec3: return 3;
        case cgltf_type_vec4: return 4;
        case cgltf_type_mat2: return 4;
        case cgltf_type_mat3: return 9;
        case cgltf_type_mat4: return 16;
        default: {
            log_error("unknown cgltf type");
            return 0;
        }
    }
}

static u8 gltf_sizeOfAccessor(cgltf_accessor* accessor)
{
    u8 componentSize = gltf_sizeOfComponentType(accessor->component_type);
    u8 numComponents = gltf_numComponentsForType(accessor->type);
    return componentSize * numComponents;
}

static SG_Geometry* geo = NULL;
static void gltf_ProcessNode(SG_Transform* parent, cgltf_node* node)
{
    SG_Transform* sg_node = createTransform();
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
    log_trace("sg_node pos: %s", glm::to_string(sg_node->pos).c_str());
    log_trace("sg_node rot: %s", glm::to_string(sg_node->rot).c_str());
    log_trace("sg_node sca: %s", glm::to_string(sg_node->sca).c_str());

    // parent-child relationship
    SG_Transform::addChild(parent, sg_node);

    // mesh
    if (node->mesh) {
        // gltf_ProcessMesh(node->mesh);
        cgltf_mesh* gltf_mesh = node->mesh;
        log_trace("processing mesh: %s", gltf_mesh->name);

        for (cgltf_size i = 0; i < gltf_mesh->primitives_count; ++i) {
            cgltf_primitive* primitive = gltf_mesh->primitives + i;
            log_trace("processing primitive %d", i);

            // TODO: cache geo data
            Vertices vertices = {};
            Vertices::init(&vertices, primitive->attributes->data->count,
                           primitive->indices->count);
            ASSERT(vertices.indicesCount > 0);

            // index buffer
            cgltf_accessor* indexAccessor      = primitive->indices;
            cgltf_buffer_view* indexBufferView = indexAccessor->buffer_view;
            cgltf_buffer* indexBuffer          = indexBufferView->buffer;
            cgltf_size indexCount              = indexAccessor->count;
            ASSERT(indexAccessor->type == cgltf_type_scalar);
            ASSERT(indexBufferView->type == cgltf_buffer_view_type_indices);

            // write indices to buffer, non-interleaved (TODO use arena?)
            u32* indices = vertices.indices;
            cgltf_size indexByteOffset
              = indexAccessor->offset + indexBufferView->offset;
            u8* indexData     = (u8*)indexBuffer->data + indexByteOffset;
            cgltf_size stride = indexBufferView->stride == 0 ?
                                  indexAccessor->stride :
                                  indexBufferView->stride;
            u8 attribByteSize = gltf_sizeOfAccessor(indexAccessor);
            // write index data to u32 array
            switch (attribByteSize) {
                case 1: {
                    for (u64 j = 0; j < indexCount; ++j) {
                        u8* index  = indexData + j * stride;
                        indices[j] = *(u8*)index;
                    }
                    break;
                }
                case 2: {
                    for (u64 j = 0; j < indexCount; ++j) {
                        u16* index = (u16*)(indexData + j * stride);
                        indices[j] = *index;
                    }
                    break;
                }
                case 4: {
                    for (u64 j = 0; j < indexCount; ++j) {
                        u32* index = (u32*)(indexData + j * stride);
                        indices[j] = *index;
                    }
                    break;
                }
                default: {
                    log_error("unsupported index size");
                    break;
                }
            }

            // print indices
            // log_trace("indices (%d):", indexCount);
            // for (u32 j = 0; j < indexCount; ++j) {
            //     printf("%d ", indices[j]);
            // }
            // printf("\n");

            // vertex attributes
            u64 vertexCount = primitive->attributes->data->count;

            for (u8 attribIndex = 0; attribIndex < primitive->attributes_count;
                 ++attribIndex) {
                cgltf_attribute* attribute
                  = primitive->attributes + attribIndex;
                log_trace("processing attribute %d [%d]: %s", attribIndex,
                          attribute->index, attribute->name);

                cgltf_accessor* accessor      = attribute->data;
                cgltf_buffer_view* bufferView = accessor->buffer_view;
                cgltf_buffer* buffer          = bufferView->buffer;
                cgltf_size count              = accessor->count;

                ASSERT(count == vertexCount);

                // get pointer to cpu buffer for this attribute
                f32* attribWriteDest = NULL;
                u8 attribComponentByteSize
                  = gltf_sizeOfComponentType(accessor->component_type);
                u8 attribNumComponents
                  = gltf_numComponentsForType(accessor->type);
                switch (attribute->type) {
                    case cgltf_attribute_type_position: {
                        attribWriteDest = Vertices::positions(&vertices);
                        ASSERT(attribNumComponents == 3);
                        break;
                    }
                    case cgltf_attribute_type_normal: {
                        attribWriteDest = Vertices::normals(&vertices);
                        ASSERT(attribNumComponents == 3);
                        break;
                    }
                    case cgltf_attribute_type_texcoord: {
                        attribWriteDest = Vertices::texcoords(&vertices);
                        ASSERT(attribNumComponents == 2);
                        break;
                    }
                    default: {
                        log_error("unsupported attribute type");
                        break;
                    }
                }

                { // write attribute to non-interleaved buffer
                    cgltf_size byteOffset
                      = accessor->offset + bufferView->offset;
                    u8* data          = (u8*)buffer->data + byteOffset;
                    cgltf_size stride = bufferView->stride == 0 ?
                                          accessor->stride :
                                          bufferView->stride;
                    // write data to buffer
                    for (u32 strideIdx = 0; strideIdx < vertexCount;
                         ++strideIdx) {
                        u8* ptr = (data + strideIdx * stride);
                        for (u8 compNum = 0; compNum < attribNumComponents;
                             ++compNum) {
                            f32 compVal = 0.0f;
                            switch (attribComponentByteSize) {
                                case 1:
                                    compVal = *(
                                      u8*)(ptr
                                           + compNum * attribComponentByteSize);
                                    break;
                                case 2:
                                    compVal
                                      = *(u16*)(ptr
                                                + compNum
                                                    * attribComponentByteSize);
                                    break;
                                case 4:
                                    compVal
                                      = *(f32*)(ptr
                                                + compNum
                                                    * attribComponentByteSize);
                                    break;
                                default:
                                    log_error("unsupported component size");
                                    break;
                            }
                            attribWriteDest[strideIdx * attribNumComponents
                                            + compNum]
                              = compVal;
                        }
                    }
                }

                // cgltf_attribute_type_position,
                // cgltf_attribute_type_normal,
                // cgltf_attribute_type_tangent,
                // cgltf_attribute_type_texcoord,
                // cgltf_attribute_type_color,
            }
            // cgltf_accessor* positionAccessor = NULL;
            // for (cgltf_size j = 0; j < primitive->attributes_count; ++j) {
            //     cgltf_attribute* attr = &primitive->attributes[j];

            //     if (attr->type == cgltf_attribute_type_position) {
            //         positionAccessor = attr->data;
            //         break;
            //     }
            // }

            // write to gpu buffer
            geo = createGeometry();
            // PlaneParams planeParams = { 1.0f, 1.0f, 1u, 1u };
            // Vertices planeVertices  = createPlane(&planeParams);
            SG_Geometry::buildFromVertices(gctx, geo, &vertices);
            // SG_Geometry::buildFromVertices(gctx, geo, &planeVertices);
            // associate transform with geometry
            geo->xformIDs.push_back(sg_node->id);

            // print vertices
            // Vertices::print(&vertices);

            break;
        } // foreach primitive
    }     // if (mesh)

    // process children
    for (cgltf_size i = 0; i < node->children_count; ++i) {
        cgltf_node* child = node->children[i];
        gltf_ProcessNode(sg_node, child);
    }
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

    RenderPipeline::init(gctx, &pipeline, shaderCode, shaderCode);

    static u8 whitePixel[4] = { 255, 255, 255, 255 };
    Texture::initSinglePixel(gctx, &texture, whitePixel);

    Material::init(gctx, &material, &pipeline, &texture);

    cgltf_options options = {};
    // Use our own memory allocation functions
    options.memory.alloc_func
      = [](void* user, cgltf_size size) { return ALLOCATE_BYTES(void, size); };
    options.memory.free_func = [](void* user, void* ptr) { FREE(ptr); };

    cgltf_data* data     = NULL;
    const char* filename = "./assets/DamagedHelmet.glb";
    cgltf_result result  = cgltf_parse_file(&options, filename, &data);
    if (result == cgltf_result_success) {
        /* TODO make awesome stuff */
        log_trace("gltf parsed successfully");
        cgltf_result buffers_result
          = cgltf_load_buffers(&options, data, filename);
        if (buffers_result != cgltf_result_success) {
            log_error("failed to load buffers");
            return;
        }
        log_trace("gltf buffer data loaded");

        SG_Transform* sg_scenes = gltf_ProcessData(data);
        for (u32 i = 0; i < data->scenes_count; ++i) {
            printf("------scene %d------\n", i);
            SG_Transform::print(sg_scenes + i);
        }

        cgltf_free(data);
    }
}

static void OnRender(glm::mat4 proj, glm::mat4 view)
{
    // std::cout << "-----basic example onRender" << std::endl;
    WGPURenderPassEncoder renderPass = GraphicsContext::prepareFrame(gctx);
    // set shader
    wgpuRenderPassEncoderSetPipeline(renderPass, pipeline.pipeline);

    // set frame uniforms
    f32 time                    = (f32)glfwGetTime();
    FrameUniforms frameUniforms = {};
    frameUniforms.projectionMat = proj;
    frameUniforms.viewMat       = view;
    frameUniforms.projViewMat
      = frameUniforms.projectionMat * frameUniforms.viewMat;
    frameUniforms.dirLight = VEC_FORWARD;
    frameUniforms.time     = time;
    wgpuQueueWriteBuffer(gctx->queue,
                         pipeline.bindGroups[PER_FRAME_GROUP].uniformBuffer, 0,
                         &frameUniforms, sizeof(frameUniforms));
    // set frame bind group
    wgpuRenderPassEncoderSetBindGroup(
      renderPass, PER_FRAME_GROUP,
      pipeline.bindGroups[PER_FRAME_GROUP].bindGroup, 0, NULL);

    // material uniforms
    MaterialUniforms materialUniforms = {};
    materialUniforms.color            = glm::vec4(1.0, 1.0, 1.0, 1.0);
    // TODO: only need to write if it's stale/changed
    wgpuQueueWriteBuffer(gctx->queue,                                //
                         material.uniformBuffer,                     //
                         0,                                          //
                         &materialUniforms, sizeof(materialUniforms) //
    );
    // set material bind groups
    wgpuRenderPassEncoderSetBindGroup(renderPass, PER_MATERIAL_GROUP,
                                      material.bindGroup, 0, NULL);

    // check indexed draw
    // bool indexedDraw = entity->vertices.indicesCount > 0;

    // set vertex attributes
    wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, geo->vertexBuffer.buf,
                                         0, sizeof(f32) * geo->numVertices * 3);

    auto normalsOffset = sizeof(f32) * geo->numVertices * 3;

    wgpuRenderPassEncoderSetVertexBuffer(renderPass, 1, geo->vertexBuffer.buf,
                                         normalsOffset,
                                         sizeof(f32) * geo->numVertices * 3);

    auto texcoordsOffset = sizeof(f32) * geo->numVertices * 6;

    wgpuRenderPassEncoderSetVertexBuffer(renderPass, 2, geo->vertexBuffer.buf,
                                         texcoordsOffset,
                                         sizeof(f32) * geo->numVertices * 2);

    // populate index buffer
    if (geo->numIndices > 0)
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, geo->indexBuffer.buf,
                                            WGPUIndexFormat_Uint32, 0,
                                            geo->indexBuffer.desc.size);
    // else
    //     wgpuRenderPassEncoderDraw(
    //       renderPass, entity->vertices.vertexCount, 1, 0, 0);

    // model uniforms TODO add instancing

    u64 numInstances            = geo->xformIDs.size();
    DrawUniforms* drawInstances = ALLOCATE_COUNT(DrawUniforms, numInstances);
    for (u32 i = 0; i < geo->xformIDs.size(); ++i) {
        SG_Transform* xform = GET_COMPONENT(SG_Transform, geo->xformIDs[i]);
        drawInstances[i].modelMat = SG_Transform::modelMatrix(xform);
    }

    // TODO: only do when xform list changes
    SG_Geometry::rebuildBindGroup(gctx, geo,
                                  pipeline.bindGroupLayouts[PER_DRAW_GROUP]);

    wgpuQueueWriteBuffer(gctx->queue, geo->storageBuffer, 0, drawInstances,
                         sizeof(DrawUniforms) * numInstances);

    // set model bind group
    wgpuRenderPassEncoderSetBindGroup(renderPass, PER_DRAW_GROUP,
                                      geo->bindGroup, 0, NULL);
    // draw call (indexed)
    if (geo->numIndices > 0) {
        wgpuRenderPassEncoderDrawIndexed(renderPass, geo->numIndices,
                                         numInstances, 0, 0, 0);
    } else {
        // draw call (nonindexed)
        wgpuRenderPassEncoderDraw(renderPass, geo->numVertices, numInstances, 0,
                                  0);
    }

    // cleanup
    FREE_ARRAY(DrawUniforms, drawInstances, numInstances);

    GraphicsContext::presentFrame(gctx);
}

void Test_Gltf(TestCallbacks* callbacks)
{
    *callbacks          = {};
    callbacks->onInit   = _Test_Gltf_OnInit;
    callbacks->onRender = OnRender;
}
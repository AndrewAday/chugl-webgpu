#include <cgltf/cgltf.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "core/log.h"
#include "core/memory.h"
#include "test_base.h"

#include "component.h"
#include "entity.h"
#include "geometry.h"
#include "shaders.h"

#include <iostream>
#include <unordered_map>
#include <vector>

static RenderPipeline pipeline = {};
static Material material       = {};
static Texture texture         = {};
static GraphicsContext* gctx   = NULL;

static SG_ID* sceneIDs;
// static SG_ID* geoIDs;

// TODO add frameArena to app or graphics context
static Arena frameArena = {};

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
    SG_Transform* sg_node = Component_CreateTransform();
    sg_node->name         = node->name;
    log_trace("processing node: %s", sg_node->name.c_str());

    // xform data
    if (node->has_translation) {
        SG_Transform::pos(sg_node, glm::make_vec3(node->translation));
    }
    if (node->has_rotation) {
        SG_Transform::rot(sg_node, glm::make_quat(node->rotation));
    }
    if (node->has_scale) {
        SG_Transform::sca(sg_node, glm::make_vec3(node->scale));
    }
    if (node->has_matrix) {
        sg_node->local = glm::make_mat4(node->matrix);
    } else {
        sg_node->local = SG_Transform::localMatrix(sg_node);
    }
    sg_node->world = parent->world * sg_node->local;

    // set xform data if gltf only provided a matrix
    if (!node->has_rotation || !node->has_scale || !node->has_translation)
        SG_Transform::setXformFromMatrix(sg_node, sg_node->local);

    // log_trace("sg_node pos: %s", glm::to_string(sg_node->pos).c_str());
    // log_trace("sg_node rot: %s", glm::to_string(sg_node->rot).c_str());
    // log_trace("sg_node sca: %s", glm::to_string(sg_node->sca).c_str());

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
            u8* gltf_index_buff = (u8*)indexBuffer->data + indexByteOffset;
            cgltf_size stride   = indexBufferView->stride == 0 ?
                                    indexAccessor->stride :
                                    indexBufferView->stride;
            u8 attribByteSize   = gltf_sizeOfAccessor(indexAccessor);

#define GLTF_COPY_INDICES(type, stride, srcByteArr, indexCount, dest)          \
    for (u32 __tmp = 0; __tmp < (indexCount); ++__tmp) {                       \
        (dest)[__tmp] = *(type*)((srcByteArr) + __tmp * (stride));             \
    }

            // write index data to u32 array
            switch (attribByteSize) {
                case 1:
                    GLTF_COPY_INDICES(u8, stride, gltf_index_buff, indexCount,
                                      indices);
                    break;
                case 2:
                    GLTF_COPY_INDICES(u16, stride, gltf_index_buff, indexCount,
                                      indices);
                    break;
                case 4:
                    GLTF_COPY_INDICES(u32, stride, gltf_index_buff, indexCount,
                                      indices);
                    break;
                default: log_error("unsupported index size"); break;
            }
#undef GLTF_COPY_INDICES

            // print indices
            // log_trace("indices (%d):", indexCount);
            // for (u32 j = 0; j < indexCount; ++j) {
            //     printf("%d ", indices[j]);
            // }
            // printf("\n");

            // vertex attributes
            u64 vertexCount = primitive->attributes->data->count;
            ASSERT(vertexCount == vertices.vertexCount);

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
                // vertex attributes should all be floats
                ASSERT(accessor->component_type == cgltf_component_type_r_32f);
                u8 attributeByteSize = gltf_sizeOfAccessor(accessor);
                u8 attribNumComponents
                  = gltf_numComponentsForType(accessor->type);

                // get the write destination
                switch (attribute->type) {
                    case cgltf_attribute_type_position: {
                        attribWriteDest = Vertices::positions(&vertices);
                        ASSERT(attribNumComponents == 3);
                        ASSERT(accessor->type == cgltf_type_vec3);
                        break;
                    }
                    case cgltf_attribute_type_normal: {
                        attribWriteDest = Vertices::normals(&vertices);
                        ASSERT(attribNumComponents == 3);
                        ASSERT(accessor->type == cgltf_type_vec3);
                        break;
                    }
                    case cgltf_attribute_type_texcoord: {
                        attribWriteDest = Vertices::texcoords(&vertices);
                        ASSERT(attribNumComponents == 2);
                        ASSERT(accessor->type == cgltf_type_vec2);

                        // TODO: handle texcoord indices
                        // e.g. TEXCOORD_0, TEXCOORD_1, etc.
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
                    // write data to buffer (assumed all float data)
                    for (u32 strideIdx = 0; strideIdx < vertexCount;
                         ++strideIdx) {
                        ASSERT(attributeByteSize
                               == sizeof(f32) * attribNumComponents);
                        memcpy(attribWriteDest
                                 + strideIdx * attribNumComponents, // dest
                               (data + strideIdx * stride),         // src
                               sizeof(f32) * attribNumComponents    // size
                        );
                    }
                }

                // cgltf_attribute_type_position,
                // cgltf_attribute_type_normal,
                // cgltf_attribute_type_tangent,
                // cgltf_attribute_type_texcoord,
                // cgltf_attribute_type_color,
            }

            // write to gpu buffer
            // TODO: store geo ids instead
            geo = Component_CreateGeometry();
            // PlaneParams planeParams = { 1.0f, 1.0f, 1u, 1u };
            // Vertices planeVertices  = createPlane(&planeParams);
            SG_Geometry::buildFromVertices(gctx, geo, &vertices);
            // SG_Geometry::buildFromVertices(gctx, geo, &planeVertices);
            // associate transform with geometry
            SG_Geometry::addXform(geo, sg_node);

            // print vertices
            // Vertices::print(&vertices);

            break; // TODO: remove. forces only 1 primitive
        }          // foreach primitive
    }              // if (mesh)

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

// appends scene ids to vector
static void gltf_ProcessData(cgltf_data* data)
{
    // process scene -- node transform hierarchy
    for (cgltf_size i = 0; i < data->scenes_count; ++i) {
        cgltf_scene* gltf_scene = &data->scenes[i];

        SG_Transform* sg_root = Component_CreateTransform();
        sg_root->name         = gltf_scene->name;
        sceneIDs[i]           = sg_root->id;

        log_trace("processing scene: %s with %d nodes", gltf_scene->name,
                  gltf_scene->nodes_count);

        for (cgltf_size j = 0; j < gltf_scene->nodes_count; ++j) {
            cgltf_node* node = gltf_scene->nodes[j];

            // process node
            gltf_ProcessNode(sg_root, node);
        }
    }
}

static void _Test_Gltf_OnInit(GraphicsContext* ctx, GLFWwindow* window)
{
    gctx = ctx;
    log_trace("gltf test init");

    Arena::init(&frameArena, MEGABYTE);

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

        // allocate memory to store scene IDs
        sceneIDs = ALLOCATE_COUNT(SG_ID, data->scenes_count);

        gltf_ProcessData(data);
        for (u32 i = 0; i < data->scenes_count; ++i) {
            printf("------scene %d------\n", i);
            SG_Transform::print(Component_GetXform(sceneIDs[i]));
        }

        cgltf_free(data);
    }
}

static void OnRender(glm::mat4 proj, glm::mat4 view)
{
    // Update all transforms
    SG_Transform::rebuildMatrices(Component_GetXform(sceneIDs[0]), &frameArena);

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
    // TODO: move into renderer, and consider separate buffer for each attribute
    // to handle case where some vertex attributes are missing (tangent, color,
    // etc)
    // TODO: what happens if a vertex attribute is not set in for the shader?
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

    // update geometry storage buffer
    SG_Geometry::rebuildBindGroup(
      gctx, geo, pipeline.bindGroupLayouts[PER_DRAW_GROUP], &frameArena);

    // set model bind group
    wgpuRenderPassEncoderSetBindGroup(renderPass, PER_DRAW_GROUP,
                                      geo->bindGroup, 0, NULL);
    // draw call (indexed)
    if (geo->numIndices > 0) {
        wgpuRenderPassEncoderDrawIndexed(
          renderPass, geo->numIndices, SG_Geometry::numInstances(geo), 0, 0, 0);
    } else {
        // draw call (nonindexed)
        wgpuRenderPassEncoderDraw(renderPass, geo->numVertices,
                                  SG_Geometry::numInstances(geo), 0, 0);
    }

    GraphicsContext::presentFrame(gctx);

    // end of frame, clear arena
    Arena::clear(&frameArena);
}

static void OnExit()
{
    log_trace("gltf test exit");

    // TODO free components
    // FREE_ARRAY(SG_ID, sceneIDs, 1);
    // FREE_ARRAY(SG_ID, geoIDs, 1);

    Arena::free(&frameArena);

    Material::release(&material);
    Texture::release(&texture);
    RenderPipeline::release(&pipeline);
}

void Test_Gltf(TestCallbacks* callbacks)
{
    *callbacks          = {};
    callbacks->onInit   = _Test_Gltf_OnInit;
    callbacks->onRender = OnRender;
    callbacks->onExit   = OnExit;
}
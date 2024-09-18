#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "core/log.h"

#include <rapidobj/rapidobj.hpp>

CK_DLL_SFUN(assloader_load_obj);

static void logRapidobjError(const rapidobj::Error& error)
{
    log_error("RapidObj error: %s", error.code.message().c_str());
    if (!error.line.empty()) {
        log_error("On line %d: \"%s\"", error.line_num, error.line.c_str());
    }
}

void ulib_assloader_query(Chuck_DL_Query* QUERY)
{
    // AssLoader --------------------------------------------------------------
    {
        BEGIN_CLASS("AssLoader", "Object");
        DOC_CLASS("Utility for asset loading; supports .obj files");

        SFUN(assloader_load_obj, "GGen", "loadObj");
        ARG("string", "filepath");
        DOC_FUNC("Load an .obj file from the given filepath");

        END_CLASS();
    }
}

// impl ============================================================================

CK_DLL_SFUN(assloader_load_obj)
{
    // for simplicity, does not support lines or points
    // renders all models with Phong lighting (for pbr use gltf loader instead)

    RETURN->v_object = NULL;
    rapidobj::Result result
      = rapidobj::ParseFile(API->object->str(GET_NEXT_STRING(ARGS)));

    if (result.error) {
        logRapidobjError(result.error);
        return;
    }

    rapidobj::Triangulate(result);

    if (result.error) {
        logRapidobjError(result.error);
        return;
    }

    result.attributes;

    // first create all unique materials
    for (const rapidobj::Material& obj_material : result.materials) {
        // obj_material.illum
    }

    // for now just use phong material for all
    SG_Material* phong_material = ulib_material_create(SG_MATERIAL_PHONG, SHRED);

    for (const rapidobj::Shape& shape : result.shapes) {
        if (!shape.lines.indices.empty()) {
            log_warn("Obj Shape \"%s\" has polylines; unsupported; skipping",
                     shape.name.c_str());
        }
        if (!shape.points.indices.empty()) {
            log_warn("Obj Shape \"%s\" has points; unsupported; skipping",
                     shape.name.c_str());
        }

        // create geometry data
        SG_Geometry* geo = ulib_geometry_create(SG_GEOMETRY, SHRED);

        result.attributes.positions;
        result.attributes.normals;
        result.attributes.texcoords;

#if 0
        { // set attribute
    ASSERT(location < SG_GEOMETRY_MAX_VERTEX_ATTRIBUTES && location >= 0);
    ASSERT(num_components >= 0);

    Arena* arena = &geo->vertex_attribute_data[location];
    Arena::clear(arena);

    // write ck_array data to arena
    f32* arena_data = ARENA_PUSH_COUNT(arena, f32, ck_arr_len);
    for (int i = 0; i < ck_arr_len; i++)
        arena_data[i] = (f32)api->object->array_float_get_idx(ck_array, i);

    // set num components
    geo->vertex_attribute_num_components[location] = num_components;

    ASSERT(ARENA_LENGTH(arena, f32) == ck_arr_len);

    return arena_data;
        }
#endif

        shape.mesh.indices;

        // shape.mesh.material_ids;

        // num_triangles += shape.mesh.num_face_vertices.size();
    }
}
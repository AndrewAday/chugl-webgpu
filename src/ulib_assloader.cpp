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

    for (const rapidobj::Shape& shape : result.shapes) {
        if (!shape.lines.indices.empty()) {
            log_warn("Obj Shape \"%s\" has polylines; unsupported; skipping",
                     shape.name.c_str());
        }
        if (!shape.points.indices.empty()) {
            log_warn("Obj Shape \"%s\" has points; unsupported; skipping",
                     shape.name.c_str());
        }

        // process mesh
        shape.mesh.indices;
        shape.mesh.material_ids;

        // num_triangles += shape.mesh.num_face_vertices.size();
    }
}
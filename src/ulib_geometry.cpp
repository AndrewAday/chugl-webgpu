
#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "geometry.h"

// ===============================================================
// Geometry  (for now, immutable)
// ===============================================================

CK_DLL_CTOR(geo_ctor);

CK_DLL_MFUN(geo_set_vertex_attribute);
CK_DLL_MFUN(geo_get_vertex_attribute_num_components);
CK_DLL_MFUN(geo_get_vertex_attribute_data);

CK_DLL_MFUN(geo_set_indices);
CK_DLL_MFUN(geo_get_indices);

CK_DLL_MFUN(geo_generate_tangents);

CK_DLL_CTOR(plane_geo_ctor);
CK_DLL_CTOR(plane_geo_ctor_params);

CK_DLL_CTOR(sphere_geo_ctor);
CK_DLL_CTOR(sphere_geo_ctor_params);

static void ulib_geometry_query(Chuck_DL_Query* QUERY)
{
    // Geometry -----------------------------------------------------
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_GEOMETRY], SG_CKNames[SG_COMPONENT_BASE]);

    // contants
    static t_CKINT sg_geometry_max_attributes{ SG_GEOMETRY_MAX_VERTEX_ATTRIBUTES };
    static t_CKINT pos_attr_loc{ SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION };
    static t_CKINT norm_attr_loc{ SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION };
    static t_CKINT uv_attr_loc{ SG_GEOMETRY_UV_ATTRIBUTE_LOCATION };
    static t_CKINT tangent_attr_loc{ SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION };

    SVAR("int", "MAX_VERTEX_ATTRIBUTES", &sg_geometry_max_attributes);
    DOC_VAR("Maximum number of vertex attributes.");

    SVAR("int", "POSITION_ATTRIBUTE_LOCATION", &pos_attr_loc);
    DOC_VAR("Position attribute location used by builtin renderer");

    SVAR("int", "NORMAL_ATTRIBUTE_LOCATION", &norm_attr_loc);
    DOC_VAR("Normal attribute location used by builtin renderer");

    SVAR("int", "UV_ATTRIBUTE_LOCATION", &uv_attr_loc);
    DOC_VAR("UV attribute location used by builtin renderer");

    SVAR("int", "TANGENT_ATTRIBUTE_LOCATION", &tangent_attr_loc);
    DOC_VAR("Tangent attribute location used by builtin renderer");

    // ctor
    CTOR(geo_ctor);

    // mfuns
    MFUN(geo_set_vertex_attribute, "void", "vertexAttribute");
    ARG("int", "location");
    ARG("int", "numComponents");
    ARG("float[]", "attributeData");
    DOC_FUNC(
      "Set the vertex attribute data for a geometry. location must be between "
      "0 and MAX_VERTEX_ATTRIBUTES.");

    MFUN(geo_get_vertex_attribute_num_components, "int",
         "vertexAttributeNumComponents");
    ARG("int", "location");
    DOC_FUNC("Get the number of components for a vertex attribute. Default 0.");

    MFUN(geo_get_vertex_attribute_data, "float[]", "vertexAttributeData");
    ARG("int", "location");
    DOC_FUNC("Get the vertex data for a vertex attribute. Default empty array.");

    MFUN(geo_set_indices, "void", "indices");
    ARG("int[]", "indices");
    DOC_FUNC("Set the indices for a geometry.");

    MFUN(geo_get_indices, "int[]", "indices");
    DOC_FUNC("Get the indices for a geometry.");

    MFUN(geo_generate_tangents, "void", "generateTangents");
    DOC_FUNC(
      "Generate tangents assuming default vertex attribute locations: "
      "position=0, normal=1, uv=2, tangent=3. Tangents are written to location = 3."
      "Indices are supported but optional."
      " Call *after* all other attribute data and indices have been supplied.");

    END_CLASS();

    // Plane -----------------------------------------------------
    QUERY->begin_class(QUERY, "PlaneGeometry", SG_CKNames[SG_COMPONENT_GEOMETRY]);

    QUERY->add_ctor(QUERY, plane_geo_ctor);
    QUERY->add_ctor(QUERY, plane_geo_ctor_params);
    QUERY->add_arg(QUERY, "float", "width");
    QUERY->add_arg(QUERY, "float", "height");
    QUERY->add_arg(QUERY, "int", "widthSegments");
    QUERY->add_arg(QUERY, "int", "heightSegments");
    QUERY->doc_func(QUERY, "Set plane dimensions and subdivisions");

    QUERY->end_class(QUERY);

    // Sphere -----------------------------------------------------
    QUERY->begin_class(QUERY, "SphereGeometry", SG_CKNames[SG_COMPONENT_GEOMETRY]);

    QUERY->add_ctor(QUERY, sphere_geo_ctor);
    QUERY->add_ctor(QUERY, sphere_geo_ctor_params);
    QUERY->add_arg(QUERY, "float", "radius");
    QUERY->add_arg(QUERY, "int", "widthSeg");
    QUERY->add_arg(QUERY, "int", "heightSeg");
    QUERY->add_arg(QUERY, "float", "phiStart");
    QUERY->add_arg(QUERY, "float", "phiLength");
    QUERY->add_arg(QUERY, "float", "thetaStart");
    QUERY->add_arg(QUERY, "float", "thetaLength");
    QUERY->doc_func(QUERY, "Set sphere dimensions and subdivisions");

    QUERY->end_class(QUERY);
}

// Geometry -----------------------------------------------------

CK_DLL_CTOR(geo_ctor)
{
    SG_Geometry* geo                           = SG_CreateGeometry(SELF);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = geo->id;
    CQ_PushCommand_GeometryCreate(geo);
}

CK_DLL_MFUN(geo_set_vertex_attribute)
{
    t_CKINT location         = GET_NEXT_INT(ARGS);
    t_CKINT num_components   = GET_NEXT_INT(ARGS);
    Chuck_ArrayFloat* ck_arr = GET_NEXT_FLOAT_ARRAY(ARGS);

    /*
    idea: pass ck array of doubles into SG_Geometry::setAttribute directly.
    have SG_Geometry copy and convert f64 -> f32

    then push to command queue, passing a pointer to this f32 array

    the command queue copies the array into the command queue arena

    renderer writes data from command queue arena to GPU buffer and DOES NOT
    need to free because arena memory is cleared on each frame
    */

    t_CKINT ck_arr_len = API->object->array_float_size(ck_arr);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    // set attribute locally
    f32* vertex_attrib_data = SG_Geometry::setAttribute(geo, location, num_components,
                                                        API, ck_arr, ck_arr_len);

    // push attribute change to command queue
    CQ_PushCommand_GeometrySetVertexAttribute(geo, location, num_components,
                                              vertex_attrib_data, ck_arr_len);
}

CK_DLL_MFUN(geo_get_vertex_attribute_num_components)
{
    t_CKINT location = GET_NEXT_INT(ARGS);
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    RETURN->v_int = geo->vertex_attribute_num_components[location];
}

CK_DLL_MFUN(geo_get_vertex_attribute_data)
{
    t_CKINT location = GET_NEXT_INT(ARGS);
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    f32* data = SG_Geometry::getAttributeData(geo, location);
    int data_count
      = SG_Geometry::vertexCount(geo) * geo->vertex_attribute_num_components[location];

    Chuck_ArrayFloat* ck_arr
      = (Chuck_ArrayFloat*)chugin_createCkObj("float[]", false, SHRED);
    for (int i = 0; i < data_count; i++)
        API->object->array_float_push_back(ck_arr, data[i]);

    RETURN->v_object = (Chuck_Object*)ck_arr;
}

CK_DLL_MFUN(geo_set_indices)
{
    Chuck_ArrayInt* ck_arr = GET_NEXT_INT_ARRAY(ARGS);
    t_CKINT ck_arr_len     = API->object->array_int_size(ck_arr);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    u32* indices = SG_Geometry::setIndices(geo, API, ck_arr, ck_arr_len);

    CQ_PushCommand_GeometrySetIndices(geo, indices, ck_arr_len);
}

CK_DLL_MFUN(geo_get_indices)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    u32* indices    = SG_Geometry::getIndices(geo);
    int index_count = SG_Geometry::indexCount(geo);

    Chuck_ArrayInt* ck_arr = (Chuck_ArrayInt*)chugin_createCkObj("int[]", false, SHRED);
    for (int i = 0; i < index_count; i++)
        API->object->array_int_push_back(ck_arr, indices[i]);

    RETURN->v_object = (Chuck_Object*)ck_arr;
}

CK_DLL_MFUN(geo_generate_tangents)
{
    SG_Geometry* g = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    // check num components to make sure they match expected values
    if (g->vertex_attribute_num_components[SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION] != 3
        || g->vertex_attribute_num_components[SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION]
             != 3
        || g->vertex_attribute_num_components[SG_GEOMETRY_UV_ATTRIBUTE_LOCATION] != 2) {
        CK_THROW("tangentGenerationException",
                 "Tangent generation requires position, normal, and uv "
                 "attributes to have 3, 3, and 2 components respectively, and be set "
                 "at locations 0, 1, and 2 respectively.",
                 SHRED);
        return;
    }

    GeometryArenaBuilder b = {};
    // set arena pointers
    b.pos_arena  = &g->vertex_attribute_data[SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION];
    b.norm_arena = &g->vertex_attribute_data[SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION];
    b.uv_arena   = &g->vertex_attribute_data[SG_GEOMETRY_UV_ATTRIBUTE_LOCATION];
    b.tangent_arena = &g->vertex_attribute_data[SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION];
    b.indices_arena = &g->indices;

    // clear arenas and resize
    Arena::clear(b.tangent_arena);
    ARENA_PUSH_COUNT(b.tangent_arena, f32, SG_Geometry::vertexCount(g) * 4);

    // set num components
    g->vertex_attribute_num_components[SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION] = 4;

    // generate tangents
    Geometry_computeTangents(&b);

    // push to command queue
    CQ_PushCommand_GeometrySetVertexAttribute(g, SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION,
                                              4, (f32*)b.tangent_arena->base,
                                              ARENA_LENGTH(b.tangent_arena, f32));
}

// Plane Geometry -----------------------------------------------------

// TODO all goemetry subclasses use geometry vertex attribute interface

static void CQ_UpdateAllVertexAttributes(SG_Geometry* geo)
{ // push attribute changes to command queue
    Arena* positions_arena
      = &geo->vertex_attribute_data[SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION];
    CQ_PushCommand_GeometrySetVertexAttribute(
      geo, SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION, 3, (f32*)positions_arena->base,
      ARENA_LENGTH(positions_arena, f32));

    Arena* normals_arena
      = &geo->vertex_attribute_data[SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION];
    CQ_PushCommand_GeometrySetVertexAttribute(
      geo, SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION, 3, (f32*)normals_arena->base,
      ARENA_LENGTH(normals_arena, f32));

    Arena* uvs_arena = &geo->vertex_attribute_data[SG_GEOMETRY_UV_ATTRIBUTE_LOCATION];
    CQ_PushCommand_GeometrySetVertexAttribute(geo, SG_GEOMETRY_UV_ATTRIBUTE_LOCATION, 2,
                                              (f32*)uvs_arena->base,
                                              ARENA_LENGTH(uvs_arena, f32));

    Arena* tangents_arena
      = &geo->vertex_attribute_data[SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION];
    CQ_PushCommand_GeometrySetVertexAttribute(
      geo, SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION, 4, (f32*)tangents_arena->base,
      ARENA_LENGTH(tangents_arena, f32));

    Arena* indices_arena = &geo->indices;
    CQ_PushCommand_GeometrySetIndices(geo, (u32*)indices_arena->base,
                                      ARENA_LENGTH(indices_arena, u32));
}

CK_DLL_CTOR(plane_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    ASSERT(geo);

    PlaneParams params = {};
    // test default value initialization
    ASSERT(params.widthSegments == 1 && params.heightSegments == 1);

    SG_Geometry::buildPlane(geo, &params);

    CQ_UpdateAllVertexAttributes(geo);
}

CK_DLL_CTOR(plane_geo_ctor_params)
{
    PlaneParams params    = {};
    params.width          = GET_NEXT_FLOAT(ARGS);
    params.height         = GET_NEXT_FLOAT(ARGS);
    params.widthSegments  = GET_NEXT_INT(ARGS);
    params.heightSegments = GET_NEXT_INT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    SG_Geometry::buildPlane(geo, &params);

    CQ_UpdateAllVertexAttributes(geo);
}

CK_DLL_CTOR(sphere_geo_ctor)
{
    SG_Geometry* geo    = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    SphereParams params = {};
    SG_Geometry::buildSphere(geo, &params);
    CQ_UpdateAllVertexAttributes(geo);
}

CK_DLL_CTOR(sphere_geo_ctor_params)
{
    SphereParams params = {};
    params.radius       = GET_NEXT_FLOAT(ARGS);
    params.widthSeg     = GET_NEXT_INT(ARGS);
    params.heightSeg    = GET_NEXT_INT(ARGS);
    params.phiStart     = GET_NEXT_FLOAT(ARGS);
    params.phiLength    = GET_NEXT_FLOAT(ARGS);
    params.thetaStart   = GET_NEXT_FLOAT(ARGS);
    params.thetaLength  = GET_NEXT_FLOAT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    SG_Geometry::buildSphere(geo, &params);
    CQ_UpdateAllVertexAttributes(geo);
}
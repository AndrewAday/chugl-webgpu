
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

CK_DLL_CTOR(plane_geo_ctor);
CK_DLL_CTOR(plane_geo_ctor_params);

CK_DLL_CTOR(sphere_geo_ctor);
CK_DLL_CTOR(sphere_geo_ctor_params);

static void ulib_geometry_query(Chuck_DL_Query* QUERY)
{
    // Geometry -----------------------------------------------------
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_GEOMETRY],
                SG_CKNames[SG_COMPONENT_BASE]);

    // contants
    static t_CKINT sg_geometry_max_attributes{ SG_GEOMETRY_MAX_VERTEX_ATTRIBUTES };
    SVAR("int", "MAX_VERTEX_ATTRIBUTES", &sg_geometry_max_attributes);

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
    DOC_FUNC(
      "Get the vertex data for a vertex attribute. Default empty array.");

    MFUN(geo_set_indices, "void", "indices");
    ARG("int[]", "indices");
    DOC_FUNC("Set the indices for a geometry.");

    MFUN(geo_get_indices, "int[]", "indices");
    DOC_FUNC("Get the indices for a geometry.");

    END_CLASS();

    // Plane -----------------------------------------------------
    QUERY->begin_class(QUERY, "PlaneGeometry",
                       SG_CKNames[SG_COMPONENT_GEOMETRY]);

    QUERY->add_ctor(QUERY, plane_geo_ctor);
    QUERY->add_ctor(QUERY, plane_geo_ctor_params);
    QUERY->add_arg(QUERY, "float", "width");
    QUERY->add_arg(QUERY, "float", "height");
    QUERY->add_arg(QUERY, "int", "widthSegments");
    QUERY->add_arg(QUERY, "int", "heightSegments");
    QUERY->doc_func(QUERY, "Set plane dimensions and subdivisions");

    QUERY->end_class(QUERY);

    // Sphere -----------------------------------------------------
    QUERY->begin_class(QUERY, "SphereGeometry",
                       SG_CKNames[SG_COMPONENT_GEOMETRY]);

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
    SG_Geometry* geo = SG_CreateGeometry(SELF, SG_GEOMETRY, NULL);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = geo->id;
    CQ_PushCommand_GeometryCreate(geo);
}

CK_DLL_MFUN(geo_set_vertex_attribute)
{
    t_CKINT location         = GET_NEXT_INT(ARGS);
    t_CKINT num_components   = GET_NEXT_INT(ARGS);
    Chuck_ArrayFloat* ck_arr = GET_NEXT_FLOAT_ARRAY(ARGS);

    t_CKINT ck_arr_len = API->object->array_float_size(ck_arr);

    f32* vertex_data = ALLOCATE_COUNT(f32, ck_arr_len);
    for (int i = 0; i < ck_arr_len; i++)
        vertex_data[i] = (f32)API->object->array_float_get_idx(ck_arr, i);

    SG_Geometry* geo
      = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    // set attribute locally
    SG_Geometry::setAttribute(geo, location, num_components, vertex_data,
                              ck_arr_len);

    // push attribute change to command queue
    CQ_PushCommand_GeometrySetVertexAttribute(geo, location, num_components,
                                              vertex_data, ck_arr_len);
}

CK_DLL_MFUN(geo_get_vertex_attribute_num_components)
{
    t_CKINT location = GET_NEXT_INT(ARGS);
    SG_Geometry* geo
      = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    RETURN->v_int = geo->vertex_attribute_num_components[location];
}

CK_DLL_MFUN(geo_get_vertex_attribute_data)
{
    t_CKINT location = GET_NEXT_INT(ARGS);
    SG_Geometry* geo
      = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    f32* data      = SG_Geometry::getAttributeData(geo, location);
    int data_count = SG_Geometry::vertexCount(geo)
                     * geo->vertex_attribute_num_components[location];

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

    u32* indices = ALLOCATE_COUNT(u32, ck_arr_len);
    for (int i = 0; i < ck_arr_len; i++)
        indices[i] = (u32)API->object->array_int_get_idx(ck_arr, i);

    SG_Geometry* geo
      = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    SG_Geometry::setIndices(geo, indices, ck_arr_len);

    CQ_PushCommand_GeometrySetIndices(geo, indices, ck_arr_len);
}

CK_DLL_MFUN(geo_get_indices)
{
    SG_Geometry* geo
      = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    u32* indices    = SG_Geometry::getIndices(geo);
    int index_count = SG_Geometry::indexCount(geo);

    Chuck_ArrayInt* ck_arr
      = (Chuck_ArrayInt*)chugin_createCkObj("int[]", false, SHRED);
    for (int i = 0; i < index_count; i++)
        API->object->array_int_push_back(ck_arr, indices[i]);

    RETURN->v_object = (Chuck_Object*)ck_arr;
}

// Plane Geometry -----------------------------------------------------

// TODO all goemetry subclasses use geometry vertex attribute interface

CK_DLL_CTOR(plane_geo_ctor)
{
    PlaneParams params = {};
    // test default value initialization
    ASSERT(params.widthSegments == 1 && params.heightSegments == 1);

    SG_Geometry* geo = SG_CreateGeometry(SELF, SG_GEOMETRY_PLANE, &params);
    ASSERT(geo->type == SG_COMPONENT_GEOMETRY);
    ASSERT(geo->id > 0);
    ASSERT(geo->geo_type == SG_GEOMETRY_PLANE);

    OBJ_MEMBER_UINT(SELF, component_offset_id) = geo->id;

    CQ_PushCommand_GeometryCreate(geo);
}

CK_DLL_CTOR(plane_geo_ctor_params)
{
    PlaneParams params    = {};
    params.width          = GET_NEXT_FLOAT(ARGS);
    params.height         = GET_NEXT_FLOAT(ARGS);
    params.widthSegments  = GET_NEXT_INT(ARGS);
    params.heightSegments = GET_NEXT_INT(ARGS);

    SG_Geometry* geo = SG_CreateGeometry(SELF, SG_GEOMETRY_PLANE, &params);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = geo->id;

    CQ_PushCommand_GeometryCreate(geo);
}

CK_DLL_CTOR(sphere_geo_ctor)
{
    SphereParams params = {};

    SG_Geometry* geo = SG_CreateGeometry(SELF, SG_GEOMETRY_SPHERE, &params);
    ASSERT(geo->type == SG_COMPONENT_GEOMETRY);
    ASSERT(geo->id > 0);
    ASSERT(geo->geo_type == SG_GEOMETRY_SPHERE);

    OBJ_MEMBER_UINT(SELF, component_offset_id) = geo->id;

    CQ_PushCommand_GeometryCreate(geo);
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

    SG_Geometry* geo = SG_CreateGeometry(SELF, SG_GEOMETRY_PLANE, &params);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = geo->id;

    CQ_PushCommand_GeometryCreate(geo);
}
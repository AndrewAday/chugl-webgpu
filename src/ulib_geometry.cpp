
#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "geometry.h"

#define GET_GEOMETRY(ckobj) SG_GetGeometry(OBJ_MEMBER_UINT(ckobj, component_offset_id))

// ===============================================================
// Geometry  (for now, immutable)
// ===============================================================
static void CQ_UpdateAllVertexAttributes(SG_Geometry* geo);

CK_DLL_CTOR(geo_ctor);

CK_DLL_MFUN(geo_set_vertex_attribute);
CK_DLL_MFUN(geo_get_vertex_attribute_num_components);
CK_DLL_MFUN(geo_get_vertex_attribute_num_components_all);
CK_DLL_MFUN(geo_get_vertex_attribute_data);

CK_DLL_MFUN(geo_set_indices);
CK_DLL_MFUN(geo_get_indices);

CK_DLL_MFUN(geo_set_vertex_count);
// CK_DLL_MFUN(geo_set_indices_count); // TODO

CK_DLL_MFUN(geo_set_pulled_vertex_attribute);
CK_DLL_MFUN(geo_set_pulled_vertex_attribute_vec2);
CK_DLL_MFUN(geo_get_pulled_vertex_attribute);

CK_DLL_MFUN(geo_generate_tangents);

CK_DLL_CTOR(plane_geo_ctor);
CK_DLL_CTOR(plane_geo_ctor_params);
CK_DLL_MFUN(plane_geo_build);
CK_DLL_MFUN(plane_geo_get_width);
CK_DLL_MFUN(plane_geo_get_height);
CK_DLL_MFUN(plane_geo_get_widthSegments);
CK_DLL_MFUN(plane_geo_get_heightSegments);

CK_DLL_CTOR(sphere_geo_ctor);
CK_DLL_CTOR(sphere_geo_ctor_params);
CK_DLL_MFUN(sphere_geo_build);
CK_DLL_MFUN(sphere_geo_get_radius);
CK_DLL_MFUN(sphere_geo_get_widthSeg);
CK_DLL_MFUN(sphere_geo_get_heightSeg);
CK_DLL_MFUN(sphere_geo_get_phiStart);
CK_DLL_MFUN(sphere_geo_get_phiLength);
CK_DLL_MFUN(sphere_geo_get_thetaStart);
CK_DLL_MFUN(sphere_geo_get_thetaLength);

CK_DLL_CTOR(suzanne_geo_ctor);

CK_DLL_CTOR(lines2d_geo_ctor_params);
CK_DLL_MFUN(lines2d_geo_set_line_points);

CK_DLL_CTOR(box_geo_ctor);
CK_DLL_CTOR(box_geo_ctor_params);
CK_DLL_MFUN(box_geo_build);
CK_DLL_MFUN(box_geo_get_width);
CK_DLL_MFUN(box_geo_get_height);
CK_DLL_MFUN(box_geo_get_depth);
CK_DLL_MFUN(box_geo_get_width_segments);
CK_DLL_MFUN(box_geo_get_height_segments);
CK_DLL_MFUN(box_geo_get_depth_segments);

CK_DLL_CTOR(circle_geo_ctor);
CK_DLL_CTOR(circle_geo_ctor_params);
CK_DLL_MFUN(circle_geo_build);
CK_DLL_MFUN(circle_geo_get_radius);
CK_DLL_MFUN(circle_geo_get_segments);
CK_DLL_MFUN(circle_geo_get_thetaStart);
CK_DLL_MFUN(circle_geo_get_thetaLength);

CK_DLL_CTOR(torus_geo_ctor);
CK_DLL_CTOR(torus_geo_ctor_params);
CK_DLL_MFUN(torus_geo_build);
CK_DLL_MFUN(torus_geo_get_radius);
CK_DLL_MFUN(torus_geo_get_tubeRadius);
CK_DLL_MFUN(torus_geo_get_radialSegments);
CK_DLL_MFUN(torus_geo_get_tubularSegments);
CK_DLL_MFUN(torus_geo_get_arcLength);

CK_DLL_CTOR(cylinder_geo_ctor);
CK_DLL_CTOR(cylinder_geo_ctor_params);
CK_DLL_MFUN(cylinder_geo_build);
CK_DLL_MFUN(cylinder_geo_get_radiusTop);
CK_DLL_MFUN(cylinder_geo_get_radiusBottom);
CK_DLL_MFUN(cylinder_geo_get_height);
CK_DLL_MFUN(cylinder_geo_get_radialSegments);
CK_DLL_MFUN(cylinder_geo_get_heightSegments);
CK_DLL_MFUN(cylinder_geo_get_openEnded);
CK_DLL_MFUN(cylinder_geo_get_thetaStart);
CK_DLL_MFUN(cylinder_geo_get_thetaLength);

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

    MFUN(geo_get_vertex_attribute_num_components_all, "int[]",
         "vertexAttributeNumComponents");
    DOC_FUNC("Get the number of components for all vertex attributes.");

    MFUN(geo_get_vertex_attribute_data, "float[]", "vertexAttributeData");
    ARG("int", "location");
    DOC_FUNC("Get the vertex data for a vertex attribute. Default empty array.");

    MFUN(geo_set_indices, "void", "indices");
    ARG("int[]", "indices");
    DOC_FUNC("Set the indices for a geometry.");

    MFUN(geo_get_indices, "int[]", "indices");
    DOC_FUNC("Get the indices for a geometry.");

    MFUN(geo_set_pulled_vertex_attribute, "void", "pulledVertexAttribute");
    ARG("int", "location");
    ARG("float[]", "data");
    DOC_FUNC(
      "Set storage buffer for programmable vertex pulling.  pulled attriubtes will be "
      "set as storage buffers bound to @group(3) in the shader. Location must be "
      "between 0-3");

    MFUN(geo_set_pulled_vertex_attribute_vec2, "void", "pulledVertexAttribute");
    ARG("int", "location");
    ARG("vec2[]", "data");
    DOC_FUNC(
      "Set storage buffer for programmable vertex pulling.  pulled attriubtes will be "
      "set as storage buffers bound to @group(3) in the shader. Location must be "
      "between 0-3");

    MFUN(geo_get_pulled_vertex_attribute, "float[]", "pulledVertexAttribute");
    ARG("int", "location");
    DOC_FUNC(
      "Get the pulled vertex attribute data at this location. location must be "
      "between 0-3");

    MFUN(geo_set_vertex_count, "void", "vertexCount");
    ARG("int", "count");
    DOC_FUNC(
      "If this geometry uses vertex pulling or non-indexed drawing, "
      "Set the number of vertices to be drawn of this geometry by the renderer"
      "Default is -1, which means all vertices are drawn. Values will be clamped to "
      "the actual number of vertices in the geometry.");

    MFUN(geo_generate_tangents, "void", "generateTangents");
    DOC_FUNC(
      "Generate tangents assuming default vertex attribute locations: "
      "position=0, normal=1, uv=2, tangent=3. Tangents are written to location = 3."
      "Indices are supported but optional."
      " Call *after* all other attribute data and indices have been supplied.");

    END_CLASS();

    // Plane -----------------------------------------------------
    QUERY->begin_class(QUERY, SG_GeometryTypeNames[SG_GEOMETRY_PLANE],
                       SG_CKNames[SG_COMPONENT_GEOMETRY]);

    QUERY->add_ctor(QUERY, plane_geo_ctor);
    QUERY->add_ctor(QUERY, plane_geo_ctor_params);
    QUERY->add_arg(QUERY, "float", "width");
    QUERY->add_arg(QUERY, "float", "height");
    QUERY->add_arg(QUERY, "int", "widthSegments");
    QUERY->add_arg(QUERY, "int", "heightSegments");
    QUERY->doc_func(QUERY, "Set plane dimensions and subdivisions");

    MFUN(plane_geo_build, "void", "build");
    ARG("float", "width");
    ARG("float", "height");
    ARG("int", "widthSegments");
    ARG("int", "heightSegments");
    DOC_FUNC("Set plane dimensions and subdivisions");

    MFUN(plane_geo_get_width, "float", "width");
    DOC_FUNC("Get the plane width.");

    MFUN(plane_geo_get_height, "float", "height");
    DOC_FUNC("Get the plane height.");

    MFUN(plane_geo_get_widthSegments, "int", "widthSegments");
    DOC_FUNC("Get the number of segments along the width.");

    MFUN(plane_geo_get_heightSegments, "int", "heightSegments");
    DOC_FUNC("Get the number of segments along the height.");

    QUERY->end_class(QUERY);

    // Sphere -----------------------------------------------------
    QUERY->begin_class(QUERY, SG_GeometryTypeNames[SG_GEOMETRY_SPHERE],
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

    QUERY->add_mfun(QUERY, sphere_geo_build, "void", "build");
    QUERY->add_arg(QUERY, "float", "radius");
    QUERY->add_arg(QUERY, "int", "widthSeg");
    QUERY->add_arg(QUERY, "int", "heightSeg");
    QUERY->add_arg(QUERY, "float", "phiStart");
    QUERY->add_arg(QUERY, "float", "phiLength");
    QUERY->add_arg(QUERY, "float", "thetaStart");
    QUERY->add_arg(QUERY, "float", "thetaLength");
    QUERY->doc_func(QUERY, "Set sphere dimensions and subdivisions");

    MFUN(sphere_geo_get_radius, "float", "radius");
    DOC_FUNC("Get the sphere radius.");

    MFUN(sphere_geo_get_widthSeg, "int", "widthSegments");
    DOC_FUNC("Get the number of segments along the width.");

    MFUN(sphere_geo_get_heightSeg, "int", "heightSegments");
    DOC_FUNC("Get the number of segments along the height.");

    MFUN(sphere_geo_get_phiStart, "float", "phiStart");
    DOC_FUNC("Get the starting angle of the sphere along the circumference.");

    MFUN(sphere_geo_get_phiLength, "float", "phiLength");
    DOC_FUNC("Get the length of the sphere along the circumference.");

    MFUN(sphere_geo_get_thetaStart, "float", "thetaStart");
    DOC_FUNC("Get the starting angle of the sphere along the central diameter.");

    MFUN(sphere_geo_get_thetaLength, "float", "thetaLength");
    DOC_FUNC("Get the length of the sphere along the central diameter.");

    QUERY->end_class(QUERY);

    // Suzanne ----------
    BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_SUZANNE],
                SG_CKNames[SG_COMPONENT_GEOMETRY]);

    CTOR(suzanne_geo_ctor);
    END_CLASS();

    // lines2d
    BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_LINES2D],
                SG_CKNames[SG_COMPONENT_GEOMETRY]);

    // CTOR(lines2d_geo_ctor_params);
    // ARG("vec2[]", "points");
    // DOC_FUNC("construct with line points");

    MFUN(lines2d_geo_set_line_points, "void", "linePoints");
    ARG("vec2[]", "points");
    DOC_FUNC("Set the line positions. Z values are fixed to 0.0");

    END_CLASS();

    // box -----------------------------------------------------
    {
        BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_BOX],
                    SG_CKNames[SG_COMPONENT_GEOMETRY]);

        CTOR(box_geo_ctor);

        CTOR(box_geo_ctor_params);
        ARG("float", "width");
        ARG("float", "height");
        ARG("float", "depth");
        ARG("int", "widthSegments");
        ARG("int", "heightSegments");
        ARG("int", "depthSegments");
        DOC_FUNC("Create box with given dimensions and subdivisions");

        MFUN(box_geo_build, "void", "build");
        ARG("float", "width");
        ARG("float", "height");
        ARG("float", "depth");
        ARG("int", "widthSegments");
        ARG("int", "heightSegments");
        ARG("int", "depthSegments");
        DOC_FUNC("Rebuild box dimensions and subdivisions");

        MFUN(box_geo_get_width, "float", "width");
        DOC_FUNC("Get the box width.");

        MFUN(box_geo_get_height, "float", "height");
        DOC_FUNC("Get the box height.");

        MFUN(box_geo_get_depth, "float", "depth");
        DOC_FUNC("Get the box depth.");

        MFUN(box_geo_get_width_segments, "int", "widthSegments");
        DOC_FUNC("Get the number of segments along the width.");

        MFUN(box_geo_get_height_segments, "int", "heightSegments");
        DOC_FUNC("Get the number of segments along the height.");

        MFUN(box_geo_get_depth_segments, "int", "depthSegments");
        DOC_FUNC("Get the number of segments along the depth.");

        END_CLASS();
    }

    // circle --------------------------------------------------------
    {

        BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_CIRCLE],
                    SG_CKNames[SG_COMPONENT_GEOMETRY]);

        CTOR(circle_geo_ctor);

        CTOR(circle_geo_ctor_params);
        ARG("float", "radius");
        ARG("int", "segments");
        ARG("float", "thetaStart");
        ARG("float", "thetaLength");

        MFUN(circle_geo_build, "void", "build");
        ARG("float", "radius");
        ARG("int", "segments");
        ARG("float", "thetaStart");
        ARG("float", "thetaLength");

        MFUN(circle_geo_get_radius, "float", "radius");
        DOC_FUNC("Get the circle radius.");

        MFUN(circle_geo_get_segments, "int", "segments");
        DOC_FUNC("Get the number of segments.");

        MFUN(circle_geo_get_thetaStart, "float", "thetaStart");
        DOC_FUNC("Get the starting angle of the circle.");

        MFUN(circle_geo_get_thetaLength, "float", "thetaLength");
        DOC_FUNC("Get the length of the circle.");

        END_CLASS();
    }

    // torus --------------------------------------------------------
    {
        BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_TORUS],
                    SG_CKNames[SG_COMPONENT_GEOMETRY]);

        CTOR(torus_geo_ctor);

        CTOR(torus_geo_ctor_params);
        ARG("float", "radius");
        ARG("float", "tubeRadius");
        ARG("int", "radialSegments");
        ARG("int", "tubularSegments");
        ARG("float", "arcLength");

        MFUN(torus_geo_build, "void", "build");
        ARG("float", "radius");
        ARG("float", "tubeRadius");
        ARG("int", "radialSegments");
        ARG("int", "tubularSegments");
        ARG("float", "arcLength");

        MFUN(torus_geo_get_radius, "float", "radius");
        DOC_FUNC("Get the torus radius.");

        MFUN(torus_geo_get_tubeRadius, "float", "tubeRadius");
        DOC_FUNC("Get the torus tube radius.");

        MFUN(torus_geo_get_radialSegments, "int", "radialSegments");
        DOC_FUNC("Get the number of radial segments.");

        MFUN(torus_geo_get_tubularSegments, "int", "tubularSegments");
        DOC_FUNC("Get the number of tubular segments.");

        MFUN(torus_geo_get_arcLength, "float", "arcLength");
        DOC_FUNC("Get the arc length.");

        END_CLASS();
    }

    // cylinder --------------------------------------------------------
    {

        BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_CYLINDER],
                    SG_CKNames[SG_COMPONENT_GEOMETRY]);

        CTOR(cylinder_geo_ctor);

        CTOR(cylinder_geo_ctor_params);
        ARG("float", "radiusTop");
        ARG("float", "radiusBottom");
        ARG("float", "height");
        ARG("int", "radialSegments");
        ARG("int", "heightSegments");
        ARG("int", "openEnded");
        ARG("float", "thetaStart");
        ARG("float", "thetaLength");

        MFUN(cylinder_geo_build, "void", "build");
        ARG("float", "radiusTop");
        ARG("float", "radiusBottom");
        ARG("float", "height");
        ARG("int", "radialSegments");
        ARG("int", "heightSegments");
        ARG("int", "openEnded");
        ARG("float", "thetaStart");
        ARG("float", "thetaLength");

        MFUN(cylinder_geo_get_radiusTop, "float", "radiusTop");
        DOC_FUNC("Get the cylinder top radius.");

        MFUN(cylinder_geo_get_radiusBottom, "float", "radiusBottom");
        DOC_FUNC("Get the cylinder bottom radius.");

        MFUN(cylinder_geo_get_height, "float", "height");
        DOC_FUNC("Get the cylinder height.");

        MFUN(cylinder_geo_get_radialSegments, "int", "radialSegments");
        DOC_FUNC("Get the number of radial segments.");

        MFUN(cylinder_geo_get_heightSegments, "int", "heightSegments");
        DOC_FUNC("Get the number of height segments.");

        MFUN(cylinder_geo_get_openEnded, "int", "openEnded");
        DOC_FUNC("Get the cylinder open ended flag.");

        MFUN(cylinder_geo_get_thetaStart, "float", "thetaStart");
        DOC_FUNC("Get the starting angle of the cylinder.");

        MFUN(cylinder_geo_get_thetaLength, "float", "thetaLength");
        DOC_FUNC("Get the length of the cylinder.");

        END_CLASS();
    }
}

// Geometry -----------------------------------------------------

// if params is NULL, uses default values
static void ulib_geometry_build(SG_Geometry* geo, void* params)
{
    switch (geo->geo_type) {
        case SG_GEOMETRY_SPHERE: {
            SphereParams p = {};
            if (params) p = *(SphereParams*)params;
            SG_Geometry::buildSphere(geo, &p);
        } break;
        case SG_GEOMETRY_PLANE: {
            PlaneParams p = {};
            if (params) p = *(PlaneParams*)params;
            SG_Geometry::buildPlane(geo, &p);
        } break;
        case SG_GEOMETRY_SUZANNE: {
            SG_Geometry::buildSuzanne(geo);
        } break;
        case SG_GEOMETRY_LINES2D: {
        } break;
        case SG_GEOMETRY_BOX: {
            BoxParams p = {};
            if (params) p = *(BoxParams*)params;
            SG_Geometry::buildBox(geo, &p);
        } break;
        case SG_GEOMETRY_CIRCLE: {
            CircleParams p = {};
            if (params) p = *(CircleParams*)params;
            SG_Geometry::buildCircle(geo, &p);
        } break;
        case SG_GEOMETRY_TORUS: {
            TorusParams p = {};
            if (params) p = *(TorusParams*)params;
            SG_Geometry::buildTorus(geo, &p);
        } break;
        case SG_GEOMETRY_CYLINDER: {
            CylinderParams p = {};
            if (params) p = *(CylinderParams*)params;
            SG_Geometry::buildCylinder(geo, &p);
        } break;
        default: ASSERT(false);
    }

    CQ_UpdateAllVertexAttributes(geo);
}

SG_Geometry* ulib_geometry_create(SG_GeometryType type, Chuck_VM_Shred* shred)
{
    CK_DL_API API = g_chuglAPI;

    ASSERT(type != SG_GEOMETRY);
    Chuck_Object* obj = chugin_createCkObj(SG_GeometryTypeNames[type], false, shred);

    SG_Geometry* geo                          = SG_CreateGeometry(obj);
    geo->geo_type                             = type;
    OBJ_MEMBER_UINT(obj, component_offset_id) = geo->id;

    CQ_PushCommand_GeometryCreate(geo);

    ulib_geometry_build(geo, NULL);

    return geo;
}

CK_DLL_CTOR(geo_ctor)
{
    SG_Geometry* geo                           = SG_CreateGeometry(SELF);
    geo->geo_type                              = SG_GEOMETRY;
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

CK_DLL_MFUN(geo_get_vertex_attribute_num_components_all)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    Chuck_ArrayInt* ck_arr
      = chugin_createCkIntArray(geo->vertex_attribute_num_components,
                                ARRAY_LENGTH(geo->vertex_attribute_num_components));

    RETURN->v_object = (Chuck_Object*)ck_arr;
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

static int geoSetPulledVertexAttribute(SG_Geometry* geo, t_CKINT location,
                                       Chuck_Object* ck_arr, int num_components)
{
    int ck_arr_len  = 0;
    f32* local_data = NULL;

    // store locally on SG_Geometry
    Arena* pull_buffer = &geo->vertex_pull_buffers[location];
    Arena::clear(pull_buffer);

    switch (num_components) {
        case 1: {
            ck_arr_len
              = g_chuglAPI->object->array_float_size((Chuck_ArrayFloat*)ck_arr);
            local_data
              = ARENA_PUSH_COUNT(pull_buffer, f32, ck_arr_len * num_components);
            chugin_copyCkFloatArray((Chuck_ArrayFloat*)ck_arr, local_data, ck_arr_len);
        } break;
        case 2: {
            ck_arr_len = g_chuglAPI->object->array_vec2_size((Chuck_ArrayVec2*)ck_arr);
            local_data
              = ARENA_PUSH_COUNT(pull_buffer, f32, ck_arr_len * num_components);
            chugin_copyCkVec2Array((Chuck_ArrayVec2*)ck_arr, local_data);
        } break;
        case 3: {
            ck_arr_len = g_chuglAPI->object->array_vec3_size((Chuck_ArrayVec3*)ck_arr);
            local_data
              = ARENA_PUSH_COUNT(pull_buffer, f32, ck_arr_len * num_components);
            chugin_copyCkVec3Array((Chuck_ArrayVec3*)ck_arr, local_data);
        } break;
        case 4: {
            ck_arr_len = g_chuglAPI->object->array_vec4_size((Chuck_ArrayVec4*)ck_arr);
            local_data
              = ARENA_PUSH_COUNT(pull_buffer, f32, ck_arr_len * num_components);
            chugin_copyCkVec4Array((Chuck_ArrayVec4*)ck_arr, local_data);
        } break;
        default: ASSERT(false);
    }

    // push attribute change to command queue
    CQ_PushCommand_GeometrySetPulledVertexAttribute(geo, location, local_data,
                                                    pull_buffer->curr);
    return ck_arr_len;
}

CK_DLL_MFUN(geo_set_pulled_vertex_attribute)
{
    t_CKINT location     = GET_NEXT_INT(ARGS);
    Chuck_Object* ck_arr = GET_NEXT_OBJECT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    geoSetPulledVertexAttribute(geo, location, ck_arr, 1);
}

CK_DLL_MFUN(geo_set_pulled_vertex_attribute_vec2)
{
    t_CKINT location     = GET_NEXT_INT(ARGS);
    Chuck_Object* ck_arr = GET_NEXT_OBJECT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    geoSetPulledVertexAttribute(geo, location, ck_arr, 2);
}

CK_DLL_MFUN(geo_get_pulled_vertex_attribute)
{
    t_CKINT location = GET_NEXT_INT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    Arena* pull_buffer = &geo->vertex_pull_buffers[location];

    RETURN->v_object = (Chuck_Object*)chugin_createCkFloatArray(
      (f32*)pull_buffer->base, ARENA_LENGTH(pull_buffer, f32));
}

CK_DLL_MFUN(geo_set_vertex_count)
{
    t_CKINT count = GET_NEXT_INT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    CQ_PushCommand_GeometrySetVertexCount(geo, count);
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
    if (indices_arena->curr > 0) {
        CQ_PushCommand_GeometrySetIndices(geo, (u32*)indices_arena->base,
                                          ARENA_LENGTH(indices_arena, u32));
    }
}

CK_DLL_CTOR(plane_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    ASSERT(geo);
    geo->geo_type = SG_GEOMETRY_PLANE;

    ulib_geometry_build(geo, NULL);
}

CK_DLL_CTOR(plane_geo_ctor_params)
{
    PlaneParams params    = {};
    params.width          = GET_NEXT_FLOAT(ARGS);
    params.height         = GET_NEXT_FLOAT(ARGS);
    params.widthSegments  = GET_NEXT_INT(ARGS);
    params.heightSegments = GET_NEXT_INT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    geo->geo_type    = SG_GEOMETRY_PLANE;

    ulib_geometry_build(geo, &params);
}

CK_DLL_MFUN(plane_geo_build)
{
    SG_Geometry* geo      = GET_GEOMETRY(SELF);
    PlaneParams params    = {};
    params.width          = GET_NEXT_FLOAT(ARGS);
    params.height         = GET_NEXT_FLOAT(ARGS);
    params.widthSegments  = GET_NEXT_INT(ARGS);
    params.heightSegments = GET_NEXT_INT(ARGS);

    ulib_geometry_build(geo, &params);
}

CK_DLL_MFUN(plane_geo_get_width)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.plane.width;
}

CK_DLL_MFUN(plane_geo_get_height)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.plane.height;
}

CK_DLL_MFUN(plane_geo_get_widthSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.plane.widthSegments;
}

CK_DLL_MFUN(plane_geo_get_heightSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.plane.heightSegments;
}

CK_DLL_CTOR(sphere_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    geo->geo_type    = SG_GEOMETRY_SPHERE;

    ulib_geometry_build(geo, NULL);
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
    geo->geo_type    = SG_GEOMETRY_SPHERE;

    ulib_geometry_build(geo, &params);
}

CK_DLL_MFUN(sphere_geo_build)
{
    SG_Geometry* geo    = GET_GEOMETRY(SELF);
    SphereParams params = {};
    params.radius       = GET_NEXT_FLOAT(ARGS);
    params.widthSeg     = GET_NEXT_INT(ARGS);
    params.heightSeg    = GET_NEXT_INT(ARGS);
    params.phiStart     = GET_NEXT_FLOAT(ARGS);
    params.phiLength    = GET_NEXT_FLOAT(ARGS);
    params.thetaStart   = GET_NEXT_FLOAT(ARGS);
    params.thetaLength  = GET_NEXT_FLOAT(ARGS);

    ulib_geometry_build(geo, &params);
}

CK_DLL_MFUN(sphere_geo_get_radius)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.sphere.radius;
}

CK_DLL_MFUN(sphere_geo_get_widthSeg)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.sphere.widthSeg;
}

CK_DLL_MFUN(sphere_geo_get_heightSeg)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.sphere.heightSeg;
}

CK_DLL_MFUN(sphere_geo_get_phiStart)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.sphere.phiStart;
}

CK_DLL_MFUN(sphere_geo_get_phiLength)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.sphere.phiLength;
}

CK_DLL_MFUN(sphere_geo_get_thetaStart)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.sphere.thetaStart;
}

CK_DLL_MFUN(sphere_geo_get_thetaLength)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.sphere.thetaLength;
}

// Suzanne Geometry -----------------------------------------------------

CK_DLL_CTOR(suzanne_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    geo->geo_type    = SG_GEOMETRY_SUZANNE;

    ulib_geometry_build(geo, NULL);
}

// Lines2D Geometry -----------------------------------------------------

CK_DLL_CTOR(lines2d_geo_ctor_params)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    geo->geo_type    = SG_GEOMETRY_LINES2D;
    // ==nice-to-have==
}

CK_DLL_MFUN(lines2d_geo_set_line_points)
{
    Chuck_Object* ck_arr = GET_NEXT_OBJECT(ARGS);
    SG_Geometry* geo     = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    int ck_arr_len = geoSetPulledVertexAttribute(geo, 0, ck_arr, 2);
    // always draw ck_arr_len+1 points to handle line loop
    CQ_PushCommand_GeometrySetVertexCount(geo, 2 * (ck_arr_len + 1));
}

// Box Geometry -----------------------------------------------------

CK_DLL_CTOR(box_geo_ctor)
{
    SG_Geometry* geo = GET_GEOMETRY(SELF);
    geo->geo_type    = SG_GEOMETRY_BOX;

    ulib_geometry_build(geo, NULL);
}

CK_DLL_CTOR(box_geo_ctor_params)
{
    BoxParams params = {};
    params.width     = GET_NEXT_FLOAT(ARGS);
    params.height    = GET_NEXT_FLOAT(ARGS);
    params.depth     = GET_NEXT_FLOAT(ARGS);
    params.widthSeg  = GET_NEXT_INT(ARGS);
    params.heightSeg = GET_NEXT_INT(ARGS);
    params.depthSeg  = GET_NEXT_INT(ARGS);

    SG_Geometry* geo = GET_GEOMETRY(SELF);
    geo->geo_type    = SG_GEOMETRY_BOX;

    ulib_geometry_build(geo, &params);
}

CK_DLL_MFUN(box_geo_build)
{
    SG_Geometry* geo = GET_GEOMETRY(SELF);
    BoxParams params = {};
    params.width     = GET_NEXT_FLOAT(ARGS);
    params.height    = GET_NEXT_FLOAT(ARGS);
    params.depth     = GET_NEXT_FLOAT(ARGS);
    params.widthSeg  = GET_NEXT_INT(ARGS);
    params.heightSeg = GET_NEXT_INT(ARGS);
    params.depthSeg  = GET_NEXT_INT(ARGS);

    ulib_geometry_build(geo, &params);
}

CK_DLL_MFUN(box_geo_get_width)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.box.width;
}

CK_DLL_MFUN(box_geo_get_height)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.box.height;
}

CK_DLL_MFUN(box_geo_get_depth)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.box.depth;
}

CK_DLL_MFUN(box_geo_get_width_segments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.box.widthSeg;
}

CK_DLL_MFUN(box_geo_get_height_segments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.box.heightSeg;
}

CK_DLL_MFUN(box_geo_get_depth_segments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.box.depthSeg;
}

// Circle Geometry -----------------------------------------------------

CK_DLL_CTOR(circle_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    geo->geo_type    = SG_GEOMETRY_CIRCLE;

    ulib_geometry_build(geo, NULL);
}

CK_DLL_CTOR(circle_geo_ctor_params)
{
    CircleParams params = {};
    params.radius       = GET_NEXT_FLOAT(ARGS);
    params.segments     = GET_NEXT_INT(ARGS);
    params.thetaStart   = GET_NEXT_FLOAT(ARGS);
    params.thetaLength  = GET_NEXT_FLOAT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    geo->geo_type    = SG_GEOMETRY_CIRCLE;

    ulib_geometry_build(geo, &params);
}

CK_DLL_MFUN(circle_geo_build)
{
    SG_Geometry* geo    = GET_GEOMETRY(SELF);
    CircleParams params = {};
    params.radius       = GET_NEXT_FLOAT(ARGS);
    params.segments     = GET_NEXT_INT(ARGS);
    params.thetaStart   = GET_NEXT_FLOAT(ARGS);
    params.thetaLength  = GET_NEXT_FLOAT(ARGS);

    ulib_geometry_build(geo, &params);
}

CK_DLL_MFUN(circle_geo_get_radius)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.circle.radius;
}

CK_DLL_MFUN(circle_geo_get_segments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.circle.segments;
}

CK_DLL_MFUN(circle_geo_get_thetaStart)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.circle.thetaStart;
}

CK_DLL_MFUN(circle_geo_get_thetaLength)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.circle.thetaLength;
}

// Torus Geometry -----------------------------------------------------

CK_DLL_CTOR(torus_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    geo->geo_type    = SG_GEOMETRY_TORUS;

    ulib_geometry_build(geo, NULL);
}

CK_DLL_CTOR(torus_geo_ctor_params)
{
    TorusParams params     = {};
    params.radius          = GET_NEXT_FLOAT(ARGS);
    params.tubeRadius      = GET_NEXT_FLOAT(ARGS);
    params.radialSegments  = GET_NEXT_INT(ARGS);
    params.tubularSegments = GET_NEXT_INT(ARGS);
    params.arcLength       = GET_NEXT_FLOAT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    geo->geo_type    = SG_GEOMETRY_TORUS;

    ulib_geometry_build(geo, &params);
}

CK_DLL_MFUN(torus_geo_build)
{
    SG_Geometry* geo       = GET_GEOMETRY(SELF);
    TorusParams params     = {};
    params.radius          = GET_NEXT_FLOAT(ARGS);
    params.tubeRadius      = GET_NEXT_FLOAT(ARGS);
    params.radialSegments  = GET_NEXT_INT(ARGS);
    params.tubularSegments = GET_NEXT_INT(ARGS);
    params.arcLength       = GET_NEXT_FLOAT(ARGS);

    ulib_geometry_build(geo, &params);
}

CK_DLL_MFUN(torus_geo_get_radius)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.torus.radius;
}

CK_DLL_MFUN(torus_geo_get_tubeRadius)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.torus.tubeRadius;
}

CK_DLL_MFUN(torus_geo_get_radialSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.torus.radialSegments;
}

CK_DLL_MFUN(torus_geo_get_tubularSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.torus.tubularSegments;
}

CK_DLL_MFUN(torus_geo_get_arcLength)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.torus.arcLength;
}

// Cylinder Geometry -----------------------------------------------------

CK_DLL_CTOR(cylinder_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    geo->geo_type    = SG_GEOMETRY_CYLINDER;

    ulib_geometry_build(geo, NULL);
}

CK_DLL_CTOR(cylinder_geo_ctor_params)
{
    CylinderParams params = {};
    params.radiusTop      = GET_NEXT_FLOAT(ARGS);
    params.radiusBottom   = GET_NEXT_FLOAT(ARGS);
    params.height         = GET_NEXT_FLOAT(ARGS);
    params.radialSegments = GET_NEXT_INT(ARGS);
    params.heightSegments = GET_NEXT_INT(ARGS);
    params.openEnded      = GET_NEXT_INT(ARGS);
    params.thetaStart     = GET_NEXT_FLOAT(ARGS);
    params.thetaLength    = GET_NEXT_FLOAT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    geo->geo_type    = SG_GEOMETRY_CYLINDER;

    ulib_geometry_build(geo, &params);
}

CK_DLL_MFUN(cylinder_geo_build)
{
    SG_Geometry* geo      = GET_GEOMETRY(SELF);
    CylinderParams params = {};
    params.radiusTop      = GET_NEXT_FLOAT(ARGS);
    params.radiusBottom   = GET_NEXT_FLOAT(ARGS);
    params.height         = GET_NEXT_FLOAT(ARGS);
    params.radialSegments = GET_NEXT_INT(ARGS);
    params.heightSegments = GET_NEXT_INT(ARGS);
    params.openEnded      = GET_NEXT_INT(ARGS);
    params.thetaStart     = GET_NEXT_FLOAT(ARGS);
    params.thetaLength    = GET_NEXT_FLOAT(ARGS);

    ulib_geometry_build(geo, &params);
}

CK_DLL_MFUN(cylinder_geo_get_radiusTop)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.cylinder.radiusTop;
}

CK_DLL_MFUN(cylinder_geo_get_radiusBottom)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.cylinder.radiusBottom;
}

CK_DLL_MFUN(cylinder_geo_get_height)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.cylinder.height;
}

CK_DLL_MFUN(cylinder_geo_get_radialSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.cylinder.radialSegments;
}

CK_DLL_MFUN(cylinder_geo_get_heightSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.cylinder.heightSegments;
}

CK_DLL_MFUN(cylinder_geo_get_openEnded)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.cylinder.openEnded ? 1 : 0;
}

CK_DLL_MFUN(cylinder_geo_get_thetaStart)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.cylinder.thetaStart;
}

CK_DLL_MFUN(cylinder_geo_get_thetaLength)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.cylinder.thetaLength;
}

#include "sg_component.h"
#include <chuck/chugin.h>

// Query function for base component class

// TODO move into ulib_base / ulib_helper class
// stores the component's SG_ID.
// CANNOT store a direct pointer because component pool may reallocate,
// invalidating all pointers
static t_CKUINT component_offset_id = 0;

CK_DLL_CTOR(component_ctor)
{
    // base class. don't allow direct instantiation
    // Chuck_DL_Api::Type componentType
    //   = API->type->lookup(VM, SG_CKNames[SG_COMPONENT_BASE]);
    // Chuck_DL_Api::Type thisType = API->object->get_type(SELF);
    // // test for conditions to create a component
    // if (API->type->is_equal(
    //       thisType,
    //       componentType) // this type is a Component (subclasses are handled
    //       on their own)
    //     || API->type->origin_hint(thisType)
    //          == ckte_origin_USERDEFINED // this type is defined .ck file
    //     || API->type->origin_hint(thisType)
    //          == ckte_origin_IMPORT // .ck file included in search path
    // ) {
    //   API->vm->throw_exception(
    //     "InstantatingAbstractBaseClassViolation",
    //     ""
    //     "Please replace this line with .nextFrame() => now;",
    //     SHRED);
    // }

    // SG_Component* component                   = new SG_Component();
    // OBJ_MEMBER_INT(SELF, component_offset_id) = (t_CKUINT)component;
}

CK_DLL_DTOR(component_dtor)
{
}

CK_DLL_MFUN(component_get_id)
{
    SG_Component* component
      = SG_GetComponent(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_uint = component->id;
}

CK_DLL_MFUN(component_get_name)
{
    SG_Component* component
      = SG_GetComponent(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_string
      = API->object->create_string(VM, component->name.c_str(), false);
}

CK_DLL_MFUN(component_set_name)
{
    SG_Component* component
      = SG_GetComponent(OBJ_MEMBER_UINT(SELF, component_offset_id));

    Chuck_String* name = GET_NEXT_STRING(ARGS);
    component->name    = API->object->str(name);

    RETURN->v_string = name;
}

static void ulib_component_query(Chuck_DL_Query* QUERY)
{
    QUERY->begin_class(QUERY, SG_CKNames[SG_COMPONENT_BASE], "Object");
    // member vars
    component_offset_id
      = QUERY->add_mvar(QUERY, "int", "@component_ptr", false);

    // member functions
    QUERY->add_mfun(QUERY, component_get_id, "int", "id");
    QUERY->add_mfun(QUERY, component_get_name, "string", "name");
    QUERY->add_mfun(QUERY, component_set_name, "string", "name");
    QUERY->add_arg(QUERY, "string", "name");

    QUERY->end_class(QUERY); // Component
}

// ============================================================================
// GGen
// ============================================================================

CK_DLL_CTOR(ggen_ctor);
CK_DLL_DTOR(ggen_dtor);

// internal
CK_DLL_MFUN(ggen_update);

// transform API
CK_DLL_MFUN(ggen_get_right);
CK_DLL_MFUN(ggen_get_forward);
CK_DLL_MFUN(ggen_get_up);

// Position
CK_DLL_MFUN(ggen_get_pos_x);
CK_DLL_MFUN(ggen_set_pos_x);

CK_DLL_MFUN(ggen_get_pos_y);
CK_DLL_MFUN(ggen_set_pos_y);

CK_DLL_MFUN(ggen_get_pos_z);
CK_DLL_MFUN(ggen_set_pos_z);

CK_DLL_MFUN(ggen_set_pos);
CK_DLL_MFUN(ggen_get_pos);

CK_DLL_MFUN(ggen_get_pos_world);
CK_DLL_MFUN(ggen_set_pos_world);

CK_DLL_MFUN(ggen_translate);
CK_DLL_MFUN(ggen_translate_x);
CK_DLL_MFUN(ggen_translate_y);
CK_DLL_MFUN(ggen_translate_z);

// Rotation
CK_DLL_MFUN(ggen_get_rot_x);
CK_DLL_MFUN(ggen_set_rot_x);

CK_DLL_MFUN(ggen_get_rot_y);
CK_DLL_MFUN(ggen_set_rot_y);

CK_DLL_MFUN(ggen_get_rot_z);
CK_DLL_MFUN(ggen_set_rot_z);

CK_DLL_MFUN(ggen_set_rot);
CK_DLL_MFUN(ggen_get_rot);

CK_DLL_MFUN(ggen_rotate);
CK_DLL_MFUN(ggen_rotate_x);
CK_DLL_MFUN(ggen_rotate_y);
CK_DLL_MFUN(ggen_rotate_z);

CK_DLL_MFUN(ggen_rot_on_local_axis);
CK_DLL_MFUN(ggen_rot_on_world_axis);

CK_DLL_MFUN(ggen_lookat_vec3);
CK_DLL_MFUN(ggen_lookat_dir);
// CK_DLL_MFUN(ggen_rotate_by);  // no rotate by design because converting from
// euler angles to quat is ambiguous

// Scale
CK_DLL_MFUN(ggen_get_scale_x);
CK_DLL_MFUN(ggen_set_scale_x);

CK_DLL_MFUN(ggen_get_scale_y);
CK_DLL_MFUN(ggen_set_scale_y);

CK_DLL_MFUN(ggen_get_scale_z);
CK_DLL_MFUN(ggen_set_scale_z);

CK_DLL_MFUN(ggen_get_scale);
CK_DLL_MFUN(ggen_set_scale);
CK_DLL_MFUN(ggen_set_scale_uniform);

CK_DLL_MFUN(ggen_get_scale_world);
CK_DLL_MFUN(ggen_set_scale_world);

// transformation matrix API
CK_DLL_MFUN(ggen_local_pos_to_world_pos);

// parent-child scenegraph API
// CK_DLL_MFUN(ggen_disconnect);
// CK_DLL_MFUN(ggen_get_parent);
// CK_DLL_MFUN(ggen_get_child_default);
// CK_DLL_MFUN(ggen_get_child);
// CK_DLL_MFUN(ggen_get_num_children);
// CK_DLL_GFUN(ggen_op_gruck);   // add child
// CK_DLL_GFUN(ggen_op_ungruck); // remove child

static void ulib_ggen_query(Chuck_DL_Query* QUERY)
{
    { // GGen
        QUERY->begin_class(QUERY, SG_CKNames[SG_COMPONENT_TRANSFORM],
                           SG_CKNames[SG_COMPONENT_BASE]);
        QUERY->add_ctor(QUERY, ggen_ctor);
        QUERY->add_dtor(QUERY, ggen_dtor);

        QUERY->add_mfun(QUERY, ggen_update, "void", "update");
        QUERY->add_arg(QUERY, "float", "dt");
        QUERY->doc_func(QUERY,
                        "This method is automatically invoked once per frame "
                        "for all GGens connected to the scene graph."
                        "Override this method in custom GGen classes to "
                        "implement your own update logic.");

        QUERY->add_mfun(QUERY, ggen_get_right, "vec3", "right");
        QUERY->doc_func(QUERY,
                        "Get the right vector of this GGen in world space");

        QUERY->add_mfun(QUERY, ggen_get_forward, "vec3", "forward");
        QUERY->doc_func(QUERY,
                        "Get the forward vector of this GGen in world space");

        QUERY->add_mfun(QUERY, ggen_get_up, "vec3", "up");
        QUERY->doc_func(QUERY, "Get the up vector of this GGen in world space");

        // Position
        // ===============================================================

        // float posX()
        QUERY->add_mfun(QUERY, ggen_get_pos_x, "float", "posX");
        QUERY->doc_func(QUERY, "Get X position of this GGen in local space");

        // float posX(float)
        QUERY->add_mfun(QUERY, ggen_set_pos_x, "float", "posX");
        QUERY->add_arg(QUERY, "float", "pos");
        QUERY->doc_func(QUERY, "Set X position of this GGen in local space");

        // float posY()
        QUERY->add_mfun(QUERY, ggen_get_pos_y, "float", "posY");
        QUERY->doc_func(QUERY, "Get Y position of this GGen in local space");

        // float posY(float)
        QUERY->add_mfun(QUERY, ggen_set_pos_y, "float", "posY");
        QUERY->add_arg(QUERY, "float", "pos");
        QUERY->doc_func(QUERY, "Set Y position of this GGen in local space");

        // float posZ()
        QUERY->add_mfun(QUERY, ggen_get_pos_z, "float", "posZ");
        QUERY->doc_func(QUERY, "Get Z position of this GGen in local space");

        // float posZ(float)
        QUERY->add_mfun(QUERY, ggen_set_pos_z, "float", "posZ");
        QUERY->add_arg(QUERY, "float", "pos");
        QUERY->doc_func(QUERY, "Set Z position of this GGen in local space");

        // vec3 pos()
        QUERY->add_mfun(QUERY, ggen_get_pos, "vec3", "pos");
        QUERY->doc_func(QUERY, "Get object position in local space");

        // vec3 pos( vec3 )
        QUERY->add_mfun(QUERY, ggen_set_pos, "vec3", "pos");
        QUERY->add_arg(QUERY, "vec3", "pos");
        QUERY->doc_func(QUERY, "Set object position in local space");

        // vec3 posWorld()
        QUERY->add_mfun(QUERY, ggen_get_pos_world, "vec3", "posWorld");
        QUERY->doc_func(QUERY, "Get object position in world space");

        // vec3 posWorld( float )
        QUERY->add_mfun(QUERY, ggen_set_pos_world, "vec3", "posWorld");
        QUERY->add_arg(QUERY, "vec3", "pos");
        QUERY->doc_func(QUERY, "Set object position in world space");

        // GGen translate( vec3 )
        QUERY->add_mfun(QUERY, ggen_translate, "GGen", "translate");
        QUERY->add_arg(QUERY, "vec3", "translation");
        QUERY->doc_func(QUERY, "Translate this GGen by the given vector");

        // GGen translateX( float )
        QUERY->add_mfun(QUERY, ggen_translate_x, "GGen", "translateX");
        QUERY->add_arg(QUERY, "float", "amt");
        QUERY->doc_func(
          QUERY,
          "Translate this GGen by given amount on the X axis in local space");

        // GGen translateY( float )
        QUERY->add_mfun(QUERY, ggen_translate_y, "GGen", "translateY");
        QUERY->add_arg(QUERY, "float", "amt");
        QUERY->doc_func(
          QUERY,
          "Translate this GGen by given amount on the Y axis in local space");

        // GGen translateZ( float )
        QUERY->add_mfun(QUERY, ggen_translate_z, "GGen", "translateZ");
        QUERY->add_arg(QUERY, "float", "amt");
        QUERY->doc_func(
          QUERY,
          "Translate this GGen by given amount on the Z axis in local space");

        // Rotation
        // ===============================================================

        // float rotX()
        QUERY->add_mfun(QUERY, ggen_get_rot_x, "float", "rotX");
        QUERY->doc_func(
          QUERY, "Get the rotation of this GGen on the X axis in local space");

        // float rotX( float )
        QUERY->add_mfun(QUERY, ggen_set_rot_x, "float", "rotX");
        QUERY->add_arg(QUERY, "float", "radians");
        QUERY->doc_func(QUERY,
                        "Set the rotation of this GGen on the X axis in local "
                        "space to the given radians");

        // float rotY()
        QUERY->add_mfun(QUERY, ggen_get_rot_y, "float", "rotY");
        QUERY->doc_func(
          QUERY, "Get the rotation of this GGen on the Y axis in local space");

        // float rotY( float )
        QUERY->add_mfun(QUERY, ggen_set_rot_y, "float", "rotY");
        QUERY->add_arg(QUERY, "float", "radians");
        QUERY->doc_func(QUERY,
                        "Set the rotation of this GGen on the Y axis in local "
                        "space to the given radians");

        // float rotZ()
        QUERY->add_mfun(QUERY, ggen_get_rot_z, "float", "rotZ");
        QUERY->doc_func(
          QUERY, "Get the rotation of this GGen on the Z axis in local space");

        // float rotZ( float )
        QUERY->add_mfun(QUERY, ggen_set_rot_z, "float", "rotZ");
        QUERY->add_arg(QUERY, "float", "radians");
        QUERY->doc_func(QUERY,
                        "Set the rotation of this GGen on the Z axis in local "
                        "space to the given radians");

        // vec3 rot()
        QUERY->add_mfun(QUERY, ggen_get_rot, "vec3", "rot");
        QUERY->doc_func(
          QUERY,
          "Get object rotation in local space as euler angles in radians");

        // vec3 rot( vec3 )
        QUERY->add_mfun(QUERY, ggen_set_rot, "vec3", "rot");
        QUERY->add_arg(QUERY, "vec3", "eulers");
        QUERY->doc_func(
          QUERY, "Set rotation of this GGen in local space as euler angles");

        // GGen rotate( vec3 )
        QUERY->add_mfun(QUERY, ggen_rotate, "GGen", "rotate");
        QUERY->add_arg(QUERY, "vec3", "eulers");
        QUERY->doc_func(
          QUERY, "Rotate this GGen by the given euler angles in local space");

        // GGen rotateX( float )
        QUERY->add_mfun(QUERY, ggen_rotate_x, "GGen", "rotateX");
        QUERY->add_arg(QUERY, "float", "radians");
        QUERY->doc_func(
          QUERY,
          "Rotate this GGen by the given radians on the X axis in local space");

        // GGen rotateY( float )
        QUERY->add_mfun(QUERY, ggen_rotate_y, "GGen", "rotateY");
        QUERY->add_arg(QUERY, "float", "radians");
        QUERY->doc_func(
          QUERY,
          "Rotate this GGen by the given radians on the Y axis in local space");

        // GGen rotateZ( float )
        QUERY->add_mfun(QUERY, ggen_rotate_z, "GGen", "rotateZ");
        QUERY->add_arg(QUERY, "float", "radians");
        QUERY->doc_func(
          QUERY,
          "Rotate this GGen by the given radians on the Z axis in local space");

        // GGen rotateOnLocalAxis( vec3, float )
        QUERY->add_mfun(QUERY, ggen_rot_on_local_axis, "GGen",
                        "rotateOnLocalAxis");
        QUERY->add_arg(QUERY, "vec3", "axis");
        QUERY->add_arg(QUERY, "float", "radians");
        QUERY->doc_func(QUERY,
                        "Rotate this GGen by the given radians on the given "
                        "axis in local space");

        // GGen rotateOnWorldAxis( vec3, float )
        QUERY->add_mfun(QUERY, ggen_rot_on_world_axis, "GGen",
                        "rotateOnWorldAxis");
        QUERY->add_arg(QUERY, "vec3", "axis");
        QUERY->add_arg(QUERY, "float", "radians");
        QUERY->doc_func(QUERY,
                        "Rotate this GGen by the given radians on the given "
                        "axis in world space");

        // GGen lookAt( vec3 )
        QUERY->add_mfun(QUERY, ggen_lookat_vec3, "GGen", "lookAt");
        QUERY->add_arg(QUERY, "vec3", "pos");
        QUERY->doc_func(QUERY, "Look at the given position in world space");

        // vec3 lookAtDir()
        QUERY->add_mfun(QUERY, ggen_get_forward, "vec3", "lookAtDir");
        QUERY->doc_func(QUERY,
                        "Get the direction this GGen is looking, i.e. the "
                        "forward vector of this GGen in world space");

        // Scale ===============================================================

        // float scaX()
        QUERY->add_mfun(QUERY, ggen_get_scale_x, "float", "scaX");
        QUERY->doc_func(QUERY, "Get X scale of this GGen in local space");

        // float scaX( float )
        QUERY->add_mfun(QUERY, ggen_set_scale_x, "float", "scaX");
        QUERY->add_arg(QUERY, "float", "scale");
        QUERY->doc_func(QUERY, "Set X scale of this GGen in local space");

        // float scaY()
        QUERY->add_mfun(QUERY, ggen_get_scale_y, "float", "scaY");
        QUERY->doc_func(QUERY, "Get Y scale of this GGen in local space");

        // float scaY( float )
        QUERY->add_mfun(QUERY, ggen_set_scale_y, "float", "scaY");
        QUERY->add_arg(QUERY, "float", "scale");
        QUERY->doc_func(QUERY, "Set Y scale of this GGen in local space");

        // float scaZ()
        QUERY->add_mfun(QUERY, ggen_get_scale_z, "float", "scaZ");
        QUERY->doc_func(QUERY, "Get Z scale of this GGen in local space");

        // float scaZ( float )
        QUERY->add_mfun(QUERY, ggen_set_scale_z, "float", "scaZ");
        QUERY->add_arg(QUERY, "float", "scale");
        QUERY->doc_func(QUERY, "Set Z scale of this GGen in local space");

        // vec3 sca()
        QUERY->add_mfun(QUERY, ggen_get_scale, "vec3", "sca");
        QUERY->doc_func(QUERY, "Get object scale in local space");

        // vec3 sca( vec3 )
        QUERY->add_mfun(QUERY, ggen_set_scale, "vec3", "sca");
        QUERY->add_arg(QUERY, "vec3", "scale");
        QUERY->doc_func(QUERY, "Set object scale in local space");

        // vec3 sca( float )
        QUERY->add_mfun(QUERY, ggen_set_scale_uniform, "vec3", "sca");
        QUERY->add_arg(QUERY, "float", "scale");
        QUERY->doc_func(
          QUERY, "Set object scale in local space uniformly across all axes");

        // vec3 scaWorld()
        QUERY->add_mfun(QUERY, ggen_get_scale_world, "vec3", "scaWorld");
        QUERY->doc_func(QUERY, "Get object scale in world space");

        // vec3 scaWorld( vec3 )
        QUERY->add_mfun(QUERY, ggen_set_scale_world, "vec3", "scaWorld");
        QUERY->add_arg(QUERY, "vec3", "scale");
        QUERY->doc_func(QUERY, "Set object scale in world space");

        // Matrix transform API
        // ===============================================================
        QUERY->add_mfun(QUERY, ggen_local_pos_to_world_pos, "vec3",
                        "posLocalToWorld");
        QUERY->add_arg(QUERY, "vec3", "localPos");
        QUERY->doc_func(QUERY,
                        "Transform a position in local space to world space");

        // scenegraph relationship methods
        // =======================================
        // QUERY->add_mfun(QUERY, ggen_get_parent, "GGen", "parent");
        // QUERY->doc_func(QUERY, "Get the parent of this GGen");

        // QUERY->add_mfun(QUERY, ggen_get_child, "GGen", "child");
        // QUERY->add_arg(QUERY, "int", "n");
        // QUERY->doc_func(QUERY, "Get the n'th child of this GGen");

        // QUERY->add_mfun(QUERY, ggen_get_child_default, "GGen", "child");
        // QUERY->doc_func(QUERY, "Get the 0th child of this GGen");

        // QUERY->add_mfun(QUERY, ggen_get_num_children, "int",
        // "numChildren"); QUERY->doc_func(QUERY, "Get the number of children
        // for this GGen");

        // // overload GGen --> GGen
        // QUERY->add_op_overload_binary(QUERY, ggen_op_gruck, "GGen", "-->",
        //                               "GGen", "lhs", "GGen", "rhs");

        // // overload GGen --< GGen
        // QUERY->add_op_overload_binary(QUERY, ggen_op_ungruck, "GGen", "--<",
        //                               "GGen", "lhs", "GGen", "rhs");

        QUERY->end_class(QUERY); // GGen
    }
}

// CGLObject DLL ==============================================
CK_DLL_CTOR(ggen_ctor)
{
    Chuck_DL_Api::Type ggenType
      = API->type->lookup(VM, SG_CKNames[SG_COMPONENT_TRANSFORM]);
    Chuck_DL_Api::Type thisType = API->object->get_type(SELF);
    // only create if:
    if (API->type->is_equal(
          thisType,
          ggenType) // this type is GGen (subclasses are handled on their own)
        || API->type->origin_hint(thisType)
             == ckte_origin_USERDEFINED // this type is defined .ck file
        || API->type->origin_hint(thisType)
             == ckte_origin_IMPORT // .ck file included in search path
    ) {
        SG_Transform* xform = SG_CreateTransform();
        // save SG_ID
        OBJ_MEMBER_UINT(SELF, component_offset_id) = xform->id;
        // TODO: push command
    }
}

CK_DLL_DTOR(ggen_dtor)
{
    // unregister from Shred2GGen map
    // (so we don't geta null ptr reference when the host SHRED exits and tries
    // to detach all GGens)
    // CGL::UnregisterGGenFromShred(SHRED, SELF);

    // push command to destroy this object on render thread as well
    // // CGL::PushCommand(new DestroySceneGraphNodeCommand(
    //   SELF, CGL::GetGGenDataOffset(), API, &CGL::mainScene));
}

CK_DLL_MFUN(ggen_update)
{
}

CK_DLL_MFUN(ggen_get_right)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    glm::vec3 right = SG_Transform::right(xform);
    RETURN->v_vec3  = { right.x, right.y, right.z };
}

CK_DLL_MFUN(ggen_get_forward)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    glm::vec3 vec  = SG_Transform::forward(xform);
    RETURN->v_vec3 = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(ggen_get_up)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    glm::vec3 vec  = SG_Transform::up(xform);
    RETURN->v_vec3 = { vec.x, vec.y, vec.z };
}

// Position Impl ===============================================================

CK_DLL_MFUN(ggen_get_pos_x)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_float = xform->pos.x;
}

CK_DLL_MFUN(ggen_set_pos_x)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT posX  = GET_NEXT_FLOAT(ARGS);
    xform->pos.x    = posX;
    RETURN->v_float = posX;

    // TODO command
    // // CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_pos_y)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_float = xform->pos.y;
}

CK_DLL_MFUN(ggen_set_pos_y)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT posY  = GET_NEXT_FLOAT(ARGS);
    xform->pos.y    = posY;
    RETURN->v_float = posY;

    // TODO command
    // // CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_pos_z)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_float = xform->pos.z;
}

CK_DLL_MFUN(ggen_set_pos_z)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT posZ  = GET_NEXT_FLOAT(ARGS);
    xform->pos.z    = posZ;
    RETURN->v_float = posZ;

    // TODO command
    // // CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_pos)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_vec3 = { xform->pos.x, xform->pos.y, xform->pos.z };
}

CK_DLL_MFUN(ggen_set_pos)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 vec   = GET_NEXT_VEC3(ARGS);
    xform->pos.x   = vec.x;
    xform->pos.y   = vec.y;
    xform->pos.z   = vec.z;
    RETURN->v_vec3 = vec;

    // // CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_pos_world)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    glm::vec3 vec  = SG_Transform::worldPosition(xform);
    RETURN->v_vec3 = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(ggen_set_pos_world)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
    SG_Transform::worldPosition(xform, glm::vec3(vec.x, vec.y, vec.z));
    RETURN->v_object = SELF;

    // // CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(ggen_translate)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 trans = GET_NEXT_VEC3(ARGS);
    SG_Transform::translate(xform, glm::vec3(trans.x, trans.y, trans.z));

    // add to command queue
    // CGL::PushCommand(new UpdatePositionCommand(cglObj));

    RETURN->v_object = SELF;
}

CK_DLL_MFUN(ggen_translate_x)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT amt = GET_NEXT_FLOAT(ARGS);
    SG_Transform::translate(xform, { amt, 0, 0 });

    // add to command queue
    // CGL::PushCommand(new UpdatePositionCommand(cglObj));

    RETURN->v_object = SELF;
}

CK_DLL_MFUN(ggen_translate_y)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT amt = GET_NEXT_FLOAT(ARGS);
    SG_Transform::translate(xform, { 0, amt, 0 });

    // add to command queue
    // CGL::PushCommand(new UpdatePositionCommand(cglObj));

    RETURN->v_object = SELF;
}

CK_DLL_MFUN(ggen_translate_z)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT amt = GET_NEXT_FLOAT(ARGS);
    SG_Transform::translate(xform, { 0, 0, amt });

    // add to command queue
    // CGL::PushCommand(new UpdatePositionCommand(cglObj));

    RETURN->v_object = SELF;
}

// Rotation Impl ===============================================================

CK_DLL_MFUN(ggen_get_rot_x)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_float = SG_Transform::eulerRotationRadians(xform).x;
}

CK_DLL_MFUN(ggen_set_rot_x)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT rad    = GET_NEXT_FLOAT(ARGS); // set in radians
    glm::vec3 eulers = SG_Transform::eulerRotationRadians(xform);
    eulers.x         = rad;
    xform->rot       = glm::quat(eulers);

    RETURN->v_float = rad;
    // CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_rot_y)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_float = SG_Transform::eulerRotationRadians(xform).y;
}

CK_DLL_MFUN(ggen_set_rot_y)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS); // set in radians
    // https://gamedev.stackexchange.com/questions/200292/applying-incremental-rotation-with-quaternions-flickering-or-hesitating
    // For continuous rotation, wrap rad to be in range [-PI/2, PI/2]
    // i.e. after exceeding PI/2, rad = rad - PI
    rad = glm::mod(rad + glm::half_pi<double>(), glm::pi<double>())
          - glm::half_pi<double>();
    glm::vec3 eulers = SG_Transform::eulerRotationRadians(xform);
    eulers.y         = rad;
    xform->rot       = glm::quat(eulers);

    RETURN->v_float = rad; // TODO: RETURN->v_object = SELF
    // CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_rot_z)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_float = SG_Transform::eulerRotationRadians(xform).z;
}

CK_DLL_MFUN(ggen_set_rot_z)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT rad    = GET_NEXT_FLOAT(ARGS); // set in radians
    glm::vec3 eulers = SG_Transform::eulerRotationRadians(xform);
    eulers.z         = rad;
    xform->rot       = glm::quat(eulers);

    RETURN->v_float = rad;

    // CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_rot)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    glm::vec3 vec  = SG_Transform::eulerRotationRadians(xform);
    RETURN->v_vec3 = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(ggen_set_rot)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 vec   = GET_NEXT_VEC3(ARGS);
    xform->rot     = glm::quat(glm::vec3(vec.x, vec.y, vec.z));
    RETURN->v_vec3 = vec;

    // CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_rotate)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
    SG_Transform::rotate(xform, glm::vec3(vec.x, vec.y, vec.z));
    RETURN->v_object = SELF;
    // CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_rotate_x)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
    SG_Transform::rotateX(xform, rad);
    RETURN->v_object = SELF;
    // CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_rotate_y)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
    SG_Transform::rotateY(xform, rad);
    RETURN->v_object = SELF;
    // CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_rotate_z)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
    SG_Transform::rotateZ(xform, rad);
    RETURN->v_object = SELF;
    // CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_rot_on_local_axis)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 vec  = GET_NEXT_VEC3(ARGS);
    t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
    SG_Transform::rotateOnLocalAxis(xform, glm::vec3(vec.x, vec.y, vec.z), rad);

    RETURN->v_object = SELF;

    // CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_rot_on_world_axis)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 vec  = GET_NEXT_VEC3(ARGS);
    t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
    SG_Transform::rotateOnWorldAxis(xform, glm::vec3(vec.x, vec.y, vec.z), rad);

    RETURN->v_object = SELF;

    // CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_lookat_vec3)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
    SG_Transform::lookAt(xform, glm::vec3(vec.x, vec.y, vec.z));
    RETURN->v_object = SELF;
    // CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

// Scale impl ===============================================================

CK_DLL_MFUN(ggen_get_scale_x)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_float = xform->sca.x;
}

CK_DLL_MFUN(ggen_set_scale_x)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT scaleX = GET_NEXT_FLOAT(ARGS);
    xform->sca.x     = scaleX;
    RETURN->v_float  = scaleX;
    // CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_scale_y)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_float = xform->sca.y;
}

CK_DLL_MFUN(ggen_set_scale_y)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT scaleY = GET_NEXT_FLOAT(ARGS);
    xform->sca.y     = scaleY;
    RETURN->v_float  = scaleY;
    // CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_scale_z)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_float = xform->sca.z;
}

CK_DLL_MFUN(ggen_set_scale_z)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT scaleZ = GET_NEXT_FLOAT(ARGS);
    xform->sca.z     = scaleZ;
    RETURN->v_float  = scaleZ;
    // CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_scale)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    RETURN->v_vec3 = { xform->sca.x, xform->sca.y, xform->sca.z };
}

CK_DLL_MFUN(ggen_set_scale)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 vec   = GET_NEXT_VEC3(ARGS);
    xform->sca     = glm::vec3(vec.x, vec.y, vec.z);
    RETURN->v_vec3 = vec;
    // CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(ggen_set_scale_uniform)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKFLOAT s    = GET_NEXT_FLOAT(ARGS);
    xform->sca.x   = s;
    xform->sca.y   = s;
    xform->sca.z   = s;
    RETURN->v_vec3 = { s, s, s };
    // CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_scale_world)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    glm::vec3 vec  = SG_Transform::worldScale(xform);
    RETURN->v_vec3 = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(ggen_set_scale_world)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
    SG_Transform::worldScale(xform, glm::vec3(vec.x, vec.y, vec.z));
    RETURN->v_vec3 = vec;
    // CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

// Transformation API
// ===============================================================

CK_DLL_MFUN(ggen_local_pos_to_world_pos)
{
    SG_Transform* xform
      = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
    glm::vec3 worldPos
      = SG_Transform::worldMatrix(xform) * glm::vec4(vec.x, vec.y, vec.z, 1.0f);
    RETURN->v_vec3 = { worldPos.x, worldPos.y, worldPos.z };
}

// Scenegraph Relationship Impl
// ===============================================================
// CK_DLL_GFUN(ggen_op_gruck)
// {
//     // get the arguments
//     Chuck_Object* lhs = GET_NEXT_OBJECT(ARGS);
//     Chuck_Object* rhs = GET_NEXT_OBJECT(ARGS);

//     if (!lhs || !rhs) {
//         std::string errMsg = std::string("in gruck operator: ")
//                              + (lhs ? "LHS" : "[null]") + " --> "
//                              + (rhs ? "RHS" : "[null]");
//         // nullptr exception
//         API->vm->throw_exception("NullPointerException", errMsg.c_str(),
//         SHRED); return;
//     }

//     // get internal representation
//     SceneGraphObject* LHS = CGL::GetSGO(lhs);
//     SceneGraphObject* RHS = CGL::GetSGO(rhs);

//     // command
//     // CGL::PushCommand(new RelationshipCommand(
//       RHS, LHS, RelationshipCommand::Relation::AddChild));

//     // return RHS
//     RETURN->v_object = rhs;
// }

// CK_DLL_GFUN(ggen_op_ungruck)
// {
//     // get the arguments
//     Chuck_Object* lhs = GET_NEXT_OBJECT(ARGS);
//     Chuck_Object* rhs = GET_NEXT_OBJECT(ARGS);
//     // get internal representation
//     SceneGraphObject* LHS = CGL::GetSGO(lhs);
//     SceneGraphObject* RHS = CGL::GetSGO(rhs);

//     // command
//     // CGL::PushCommand(new RelationshipCommand(
//       RHS, LHS, RelationshipCommand::Relation::RemoveChild));

//     // return RHS
//     RETURN->v_object = rhs;
// }

// CK_DLL_MFUN(ggen_get_parent)
// {
//     SG_Transform* xform
// = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
//     auto* parent             = SG_Transform::GetParent();
//     // TODO: shouldn't have to refcount here, right?
//     RETURN->v_object = parent ? parent->m_ChuckObject : nullptr;
// }

// CK_DLL_MFUN(ggen_get_child_default)
// {
//     SG_Transform* xform
// = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
//     auto& children           = SG_Transform::GetChildren();
//     RETURN->v_object = children.empty() ? nullptr :
//     children[0]->m_ChuckObject;
// }

// CK_DLL_MFUN(ggen_get_child)
// {
//     SG_Transform* xform
// = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
//     auto& children           = SG_Transform::GetChildren();
//     int n                    = GET_NEXT_INT(ARGS);
//     if (n < 0 || n >= children.size()) {
//         API->vm->em_log(1, "Warning: GGen::child() index out of bounds!\n");
//         RETURN->v_object = nullptr;
//     } else {
//         RETURN->v_object = children[n]->m_ChuckObject;
//     }
// }

// CK_DLL_MFUN(ggen_get_num_children)
// {
//     SG_Transform* xform
// = SG_GetTransform(OBJ_MEMBER_UINT(SELF, component_offset_id));
//     RETURN->v_int            = SG_Transform::GetChildren().size();
// }

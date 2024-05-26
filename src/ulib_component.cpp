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
    RETURN->v_string = ck_create_string(VM, component->name.c_str(), false);
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
    { // Component
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
CK_DLL_MFUN(ggen_get_parent);
CK_DLL_MFUN(ggen_get_child_default);
CK_DLL_MFUN(ggen_get_child);
CK_DLL_MFUN(ggen_get_num_children);
CK_DLL_GFUN(ggen_op_gruck);   // add child
CK_DLL_GFUN(ggen_op_ungruck); // remove child

static void ulib_ggen_query(Chuck_DL_Query* QUERY)
{
    { // GGen
        QUERY->begin_class(QUERY, SG_CKNames[SG_COMPONENT_TRANSFORM],
                           SG_CKNames[SG_COMPONENT_BASE]);
        QUERY->add_ctor(QUERY, ggen_ctor);
        // member vars
        // member functions
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
    // CGL::PushCommand(new DestroySceneGraphNodeCommand(
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
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    const auto& vec          = cglObj->GetForward();
    RETURN->v_vec3           = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(ggen_get_up)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    const auto& vec          = cglObj->GetUp();
    RETURN->v_vec3           = { vec.x, vec.y, vec.z };
}

// Position Impl ===============================================================

CK_DLL_MFUN(ggen_get_pos_x)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    RETURN->v_float          = cglObj->GetPosition().x;
}

CK_DLL_MFUN(ggen_set_pos_x)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT posX           = GET_NEXT_FLOAT(ARGS);
    glm::vec3 pos            = cglObj->GetPosition();
    pos.x                    = posX;
    cglObj->SetPosition(pos);
    RETURN->v_float = posX;
    CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_pos_y)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    RETURN->v_float          = cglObj->GetPosition().y;
}

CK_DLL_MFUN(ggen_set_pos_y)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT posY           = GET_NEXT_FLOAT(ARGS);
    glm::vec3 pos            = cglObj->GetPosition();
    pos.y                    = posY;
    cglObj->SetPosition(pos);
    RETURN->v_float = posY;
    CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_pos_z)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    RETURN->v_float          = cglObj->GetPosition().z;
}

CK_DLL_MFUN(ggen_set_pos_z)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT posZ           = GET_NEXT_FLOAT(ARGS);
    glm::vec3 pos            = cglObj->GetPosition();
    pos.z                    = posZ;
    cglObj->SetPosition(pos);
    RETURN->v_float = posZ;
    CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_pos)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    const auto& vec          = cglObj->GetPosition();
    RETURN->v_vec3           = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(ggen_set_pos)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKVEC3 vec             = GET_NEXT_VEC3(ARGS);
    cglObj->SetPosition(glm::vec3(vec.x, vec.y, vec.z));
    RETURN->v_vec3 = vec;
    CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_pos_world)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    const auto& vec          = cglObj->GetWorldPosition();
    RETURN->v_vec3           = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(ggen_set_pos_world)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKVEC3 vec             = GET_NEXT_VEC3(ARGS);
    cglObj->SetWorldPosition(glm::vec3(vec.x, vec.y, vec.z));
    RETURN->v_object = SELF;
    CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(ggen_translate)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKVEC3 trans           = GET_NEXT_VEC3(ARGS);
    cglObj->Translate(glm::vec3(trans.x, trans.y, trans.z));

    // add to command queue
    CGL::PushCommand(new UpdatePositionCommand(cglObj));

    RETURN->v_object = SELF;
}

CK_DLL_MFUN(ggen_translate_x)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT amt            = GET_NEXT_FLOAT(ARGS);
    cglObj->Translate({ amt, 0, 0 });

    // add to command queue
    CGL::PushCommand(new UpdatePositionCommand(cglObj));

    RETURN->v_object = SELF;
}

CK_DLL_MFUN(ggen_translate_y)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT amt            = GET_NEXT_FLOAT(ARGS);
    cglObj->Translate({ 0, amt, 0 });

    // add to command queue
    CGL::PushCommand(new UpdatePositionCommand(cglObj));

    RETURN->v_object = SELF;
}

CK_DLL_MFUN(ggen_translate_z)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT amt            = GET_NEXT_FLOAT(ARGS);
    cglObj->Translate({ 0, 0, amt });

    // add to command queue
    CGL::PushCommand(new UpdatePositionCommand(cglObj));

    RETURN->v_object = SELF;
}

// Rotation Impl ===============================================================

CK_DLL_MFUN(ggen_get_rot_x)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    RETURN->v_float          = cglObj->GetRotation().x;
}

CK_DLL_MFUN(ggen_set_rot_x)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT rad            = GET_NEXT_FLOAT(ARGS);
    auto eulers              = cglObj->GetEulerRotationRadians();
    eulers.x                 = rad;
    cglObj->SetRotation(eulers);
    RETURN->v_float = rad;
    CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_rot_y)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    RETURN->v_float          = cglObj->GetRotation().y;
}

CK_DLL_MFUN(ggen_set_rot_y)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT rad            = GET_NEXT_FLOAT(ARGS);
    auto eulers              = cglObj->GetEulerRotationRadians();
    // https://gamedev.stackexchange.com/questions/200292/applying-incremental-rotation-with-quaternions-flickering-or-hesitating
    // For continuous rotation, wrap rad to be in range [-PI/2, PI/2]
    // i.e. after exceeding PI/2, rad = rad - PI
    rad = glm::mod(rad + glm::half_pi<double>(), glm::pi<double>())
          - glm::half_pi<double>();

    eulers.y = rad;
    cglObj->SetRotation(eulers);
    RETURN->v_float = rad;
    CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_rot_z)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    RETURN->v_float          = cglObj->GetRotation().z;
}

CK_DLL_MFUN(ggen_set_rot_z)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT rad            = GET_NEXT_FLOAT(ARGS);
    auto eulers              = cglObj->GetEulerRotationRadians();
    eulers.z                 = rad;
    cglObj->SetRotation(eulers);
    RETURN->v_float = rad;
    CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_rot)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    const auto& vec          = cglObj->GetEulerRotationRadians();
    RETURN->v_vec3           = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(ggen_set_rot)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKVEC3 vec             = GET_NEXT_VEC3(ARGS);
    cglObj->SetRotation(glm::vec3(vec.x, vec.y, vec.z));
    RETURN->v_vec3 = vec;
    CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_rotate)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKVEC3 vec             = GET_NEXT_VEC3(ARGS);
    cglObj->Rotate(glm::vec3(vec.x, vec.y, vec.z));
    RETURN->v_object = SELF;
    CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_rotate_x)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT rad            = GET_NEXT_FLOAT(ARGS);
    cglObj->RotateX(rad);
    RETURN->v_object = SELF;
    CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_rotate_y)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT rad            = GET_NEXT_FLOAT(ARGS);
    cglObj->RotateY(rad);
    RETURN->v_object = SELF;
    CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_rotate_z)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT rad            = GET_NEXT_FLOAT(ARGS);
    cglObj->RotateZ(rad);
    RETURN->v_object = SELF;
    CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(ggen_rot_on_local_axis)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKVEC3 vec             = GET_NEXT_VEC3(ARGS);
    t_CKFLOAT deg            = GET_NEXT_FLOAT(ARGS);
    cglObj->RotateOnLocalAxis(glm::vec3(vec.x, vec.y, vec.z), deg);

    CGL::PushCommand(new UpdateRotationCommand(cglObj));

    RETURN->v_object = SELF;
}

CK_DLL_MFUN(ggen_rot_on_world_axis)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKVEC3 vec             = GET_NEXT_VEC3(ARGS);
    t_CKFLOAT deg            = GET_NEXT_FLOAT(ARGS);
    cglObj->RotateOnWorldAxis(glm::vec3(vec.x, vec.y, vec.z), deg);

    CGL::PushCommand(new UpdateRotationCommand(cglObj));

    RETURN->v_object = SELF;
}

CK_DLL_MFUN(ggen_lookat_vec3)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKVEC3 vec             = GET_NEXT_VEC3(ARGS);
    cglObj->LookAt(glm::vec3(vec.x, vec.y, vec.z));
    RETURN->v_object = SELF;
    CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

// Scale impl ===============================================================

CK_DLL_MFUN(ggen_get_scale_x)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    RETURN->v_float          = cglObj->GetScale().x;
}

CK_DLL_MFUN(ggen_set_scale_x)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT scaleX         = GET_NEXT_FLOAT(ARGS);
    glm::vec3 scale          = cglObj->GetScale();
    scale.x                  = scaleX;
    cglObj->SetScale(scale);
    RETURN->v_float = scaleX;
    CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_scale_y)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    RETURN->v_float          = cglObj->GetScale().y;
}

CK_DLL_MFUN(ggen_set_scale_y)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT scaleY         = GET_NEXT_FLOAT(ARGS);
    glm::vec3 scale          = cglObj->GetScale();
    scale.y                  = scaleY;
    cglObj->SetScale(scale);
    RETURN->v_float = scaleY;
    CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_scale_z)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    RETURN->v_float          = cglObj->GetScale().z;
}

CK_DLL_MFUN(ggen_set_scale_z)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT scaleZ         = GET_NEXT_FLOAT(ARGS);
    glm::vec3 scale          = cglObj->GetScale();
    scale.z                  = scaleZ;
    cglObj->SetScale(scale);
    RETURN->v_float = scaleZ;
    CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_scale)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    const auto& vec          = cglObj->GetScale();
    RETURN->v_vec3           = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(ggen_set_scale)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKVEC3 vec             = GET_NEXT_VEC3(ARGS);
    cglObj->SetScale(glm::vec3(vec.x, vec.y, vec.z));
    RETURN->v_vec3 = vec;
    CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(ggen_set_scale_uniform)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKFLOAT s              = GET_NEXT_FLOAT(ARGS);
    cglObj->SetScale({ s, s, s });
    RETURN->v_vec3 = { s, s, s };
    CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(ggen_get_scale_world)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    const auto& vec          = cglObj->GetWorldScale();
    RETURN->v_vec3           = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(ggen_set_scale_world)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKVEC3 vec             = GET_NEXT_VEC3(ARGS);
    cglObj->SetWorldScale(glm::vec3(vec.x, vec.y, vec.z));
    RETURN->v_vec3 = vec;
    CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

// Transformation API
// ===============================================================

CK_DLL_MFUN(ggen_local_pos_to_world_pos)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    t_CKVEC3 vec             = GET_NEXT_VEC3(ARGS);
    glm::vec3 worldPos
      = cglObj->GetWorldMatrix() * glm::vec4(vec.x, vec.y, vec.z, 1.0f);
    RETURN->v_vec3 = { worldPos.x, worldPos.y, worldPos.z };
}

// Scenegraph Relationship Impl
// ===============================================================
CK_DLL_GFUN(ggen_op_gruck)
{
    // get the arguments
    Chuck_Object* lhs = GET_NEXT_OBJECT(ARGS);
    Chuck_Object* rhs = GET_NEXT_OBJECT(ARGS);

    if (!lhs || !rhs) {
        std::string errMsg = std::string("in gruck operator: ")
                             + (lhs ? "LHS" : "[null]") + " --> "
                             + (rhs ? "RHS" : "[null]");
        // nullptr exception
        API->vm->throw_exception("NullPointerException", errMsg.c_str(), SHRED);
        return;
    }

    // get internal representation
    SceneGraphObject* LHS = CGL::GetSGO(lhs);
    SceneGraphObject* RHS = CGL::GetSGO(rhs);

    // command
    CGL::PushCommand(new RelationshipCommand(
      RHS, LHS, RelationshipCommand::Relation::AddChild));

    // return RHS
    RETURN->v_object = rhs;
}

CK_DLL_GFUN(ggen_op_ungruck)
{
    // get the arguments
    Chuck_Object* lhs = GET_NEXT_OBJECT(ARGS);
    Chuck_Object* rhs = GET_NEXT_OBJECT(ARGS);
    // get internal representation
    SceneGraphObject* LHS = CGL::GetSGO(lhs);
    SceneGraphObject* RHS = CGL::GetSGO(rhs);

    // command
    CGL::PushCommand(new RelationshipCommand(
      RHS, LHS, RelationshipCommand::Relation::RemoveChild));

    // return RHS
    RETURN->v_object = rhs;
}

CK_DLL_MFUN(ggen_get_parent)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    auto* parent             = cglObj->GetParent();
    // TODO: shouldn't have to refcount here, right?
    RETURN->v_object = parent ? parent->m_ChuckObject : nullptr;
}

CK_DLL_MFUN(ggen_get_child_default)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    auto& children           = cglObj->GetChildren();
    RETURN->v_object = children.empty() ? nullptr : children[0]->m_ChuckObject;
}

CK_DLL_MFUN(ggen_get_child)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    auto& children           = cglObj->GetChildren();
    int n                    = GET_NEXT_INT(ARGS);
    if (n < 0 || n >= children.size()) {
        API->vm->em_log(1, "Warning: GGen::child() index out of bounds!\n");
        RETURN->v_object = nullptr;
    } else {
        RETURN->v_object = children[n]->m_ChuckObject;
    }
}

CK_DLL_MFUN(ggen_get_num_children)
{
    SceneGraphObject* cglObj = CGL::GetSGO(SELF);
    RETURN->v_int            = cglObj->GetChildren().size();
}

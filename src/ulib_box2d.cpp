#include <box2d/box2d.h>
#include <chuck/chugin.h>

#include "ulib_helper.h"

static_assert(sizeof(void*) == sizeof(t_CKUINT), "pointer size mismatch");
// make sure b2 ids fit within a chuck int
static_assert(sizeof(b2WorldId) <= sizeof(t_CKINT), "b2Worldsize mismatch");
static_assert(sizeof(b2BodyId) <= sizeof(t_CKINT), "b2Body size mismatch");
static_assert(sizeof(b2ShapeId) <= sizeof(t_CKINT), "b2Shape size mismatch");
static_assert(sizeof(b2Filter) <= sizeof(t_CKVEC3), "b2Filter size mismatch");

#define b2Vec2_TO_VEC2(v)                                                      \
    {                                                                          \
        v.x, v.y                                                               \
    }

b2BodyType ckint_to_b2BodyType(t_CKINT type)
{
    switch (type) {
        case 0: return b2_staticBody;
        case 1: return b2_kinematicBody;
        case 2: return b2_dynamicBody;
        default: return b2_staticBody;
    }
}

static t_CKUINT b2_world_def_data_offset = 0;
static t_CKUINT b2_body_def_data_offset  = 0;
static t_CKUINT b2_body_data_offset      = 0;
static t_CKUINT b2_shape_data_offset     = 0;
static t_CKUINT b2_polygon_data_offset   = 0;
static t_CKUINT b2_mass_data_offset      = 0;
static t_CKUINT b2_shape_def_data_offset = 0;
static t_CKUINT b2_filter_data_offset    = 0;

// macros for setting/getting b2 data types
// need to differentiate between b2Ids that are stored directly IN chuck object
// vs larger structs that are stored as a pointer to heap memory

#define B2_GET_BODY_ID(ckobj)                                                  \
    *(b2BodyId*)OBJ_MEMBER_DATA(ckobj, b2_body_data_offset)

#define B2_GET_FILTER_PTR(ckobj)                                               \
    (b2Filter*)OBJ_MEMBER_DATA(ckobj, b2_filter_data_offset)

#define B2_GET_BODY_DEF_PTR(ckobj)                                             \
    ((b2BodyDef*)OBJ_MEMBER_UINT(ckobj, b2_body_def_data_offset))

// ============================================================================
// b2_ShapeDef
// ============================================================================

CK_DLL_CTOR(b2_ShapeDef_ctor)
{
    OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset)
      = (t_CKUINT)(new b2ShapeDef);
    b2ShapeDef default_shape_def = b2DefaultShapeDef();
    memcpy((b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset),
           &default_shape_def, sizeof(b2ShapeDef));
}

CK_DLL_DTOR(b2_ShapeDef_dtor)
{
    CHUGIN_SAFE_DELETE(b2ShapeDef, b2_shape_def_data_offset);
}

CK_DLL_MFUN(b2_ShapeDef_get_friction)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    RETURN->v_float = data->friction;
}

CK_DLL_MFUN(b2_ShapeDef_get_restitution)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    RETURN->v_float = data->restitution;
}

CK_DLL_MFUN(b2_ShapeDef_get_density)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    RETURN->v_float = data->density;
}

CK_DLL_MFUN(b2_ShapeDef_get_isSensor)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    RETURN->v_int = data->isSensor;
}

CK_DLL_MFUN(b2_ShapeDef_get_enableSensorEvents)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    RETURN->v_int = data->enableSensorEvents;
}

CK_DLL_MFUN(b2_ShapeDef_get_enableContactEvents)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    RETURN->v_int = data->enableContactEvents;
}

CK_DLL_MFUN(b2_ShapeDef_get_enableHitEvents)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    RETURN->v_int = data->enableHitEvents;
}

CK_DLL_MFUN(b2_ShapeDef_get_enablePreSolveEvents)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    RETURN->v_int = data->enablePreSolveEvents;
}

CK_DLL_MFUN(b2_ShapeDef_get_forceContactCreation)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    RETURN->v_int = data->forceContactCreation;
}

CK_DLL_MFUN(b2_ShapeDef_get_filter)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    Chuck_Object* filter_obj = chugin_createCkObj("b2_Filter", false, SHRED);
    b2Filter* filter         = B2_GET_FILTER_PTR(filter_obj);
    *filter                  = data->filter;
    RETURN->v_object         = filter_obj;
}

CK_DLL_MFUN(b2_ShapeDef_set_friction)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    data->friction = GET_NEXT_FLOAT(ARGS);
}

CK_DLL_MFUN(b2_ShapeDef_set_restitution)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    data->restitution = GET_NEXT_FLOAT(ARGS);
}

CK_DLL_MFUN(b2_ShapeDef_set_density)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    data->density = GET_NEXT_FLOAT(ARGS);
}

CK_DLL_MFUN(b2_ShapeDef_set_isSensor)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    data->isSensor = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_ShapeDef_set_enableSensorEvents)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    data->enableSensorEvents = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_ShapeDef_set_enableContactEvents)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    data->enableContactEvents = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_ShapeDef_set_enableHitEvents)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    data->enableHitEvents = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_ShapeDef_set_enablePreSolveEvents)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    data->enablePreSolveEvents = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_ShapeDef_set_forceContactCreation)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    data->forceContactCreation = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_ShapeDef_set_filter)
{
    b2ShapeDef* data
      = (b2ShapeDef*)OBJ_MEMBER_UINT(SELF, b2_shape_def_data_offset);
    Chuck_Object* filter_obj = GET_NEXT_OBJECT(ARGS);
    b2Filter* filter         = B2_GET_FILTER_PTR(filter_obj);
    data->filter             = *filter;
}

// ============================================================================
// b2MassData
// ============================================================================

CK_DLL_CTOR(b2_MassData_ctor)
{
    OBJ_MEMBER_UINT(SELF, b2_mass_data_offset) = (t_CKUINT)(new b2MassData);
}

CK_DLL_DTOR(b2_MassData_dtor)
{
    CHUGIN_SAFE_DELETE(b2MassData, b2_mass_data_offset);
}

CK_DLL_MFUN(b2_MassData_get_mass)
{
    b2MassData* data = (b2MassData*)OBJ_MEMBER_UINT(SELF, b2_mass_data_offset);
    RETURN->v_float  = data->mass;
}

CK_DLL_MFUN(b2_MassData_get_center)
{
    b2MassData* data = (b2MassData*)OBJ_MEMBER_UINT(SELF, b2_mass_data_offset);
    RETURN->v_vec2   = { data->center.x, data->center.y };
}

CK_DLL_MFUN(b2_MassData_get_I)
{
    b2MassData* data = (b2MassData*)OBJ_MEMBER_UINT(SELF, b2_mass_data_offset);
    RETURN->v_float  = data->I;
}

CK_DLL_MFUN(b2_MassData_set_mass)
{
    b2MassData* data = (b2MassData*)OBJ_MEMBER_UINT(SELF, b2_mass_data_offset);
    data->mass       = GET_NEXT_FLOAT(ARGS);
}

CK_DLL_MFUN(b2_MassData_set_center)
{
    b2MassData* data = (b2MassData*)OBJ_MEMBER_UINT(SELF, b2_mass_data_offset);
    t_CKVEC2 center_vec2 = GET_NEXT_VEC2(ARGS);
    data->center         = { (float)center_vec2.x, (float)center_vec2.y };
}

CK_DLL_MFUN(b2_MassData_set_I)
{
    b2MassData* data = (b2MassData*)OBJ_MEMBER_UINT(SELF, b2_mass_data_offset);
    data->I          = GET_NEXT_FLOAT(ARGS);
}

// ============================================================================
// b2WorldDef
// ============================================================================

CK_DLL_CTOR(b2_WorldDef_ctor)
{
    b2WorldDef* def                                 = new b2WorldDef;
    *def                                            = b2DefaultWorldDef();
    OBJ_MEMBER_UINT(SELF, b2_world_def_data_offset) = (t_CKUINT)def;
}

CK_DLL_DTOR(b2_WorldDef_dtor)
{
    // TODO put this safe delete in a macro
    b2WorldDef* def
      = (b2WorldDef*)OBJ_MEMBER_UINT(SELF, b2_world_def_data_offset);
    delete def;
    OBJ_MEMBER_UINT(SELF, b2_world_def_data_offset) = 0;
}

// ============================================================================
// b2Polygon
// ============================================================================

CK_DLL_DTOR(b2_polygon_dtor)
{
    CHUGIN_SAFE_DELETE(b2Polygon, b2_polygon_data_offset);
}

CK_DLL_SFUN(b2_polygon_make_box)
{
    float hx           = GET_NEXT_FLOAT(ARGS);
    float hy           = GET_NEXT_FLOAT(ARGS);
    b2Polygon box      = b2MakeBox(hx, hy);
    b2Polygon* box_ptr = new b2Polygon;
    memcpy(box_ptr, &box, sizeof(box));

    Chuck_Object* poly = chugin_createCkObj("b2_Polygon", false, SHRED);
    OBJ_MEMBER_UINT(poly, b2_polygon_data_offset) = (t_CKUINT)box_ptr;

    RETURN->v_object = poly;
}

// ============================================================================
// b2BodyDef
// ============================================================================

CK_DLL_CTOR(b2_body_def_ctor)
{
    b2BodyDef* def                                 = new b2BodyDef;
    *def                                           = b2DefaultBodyDef();
    OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset) = (t_CKUINT)def;
}

CK_DLL_DTOR(b2_body_def_dtor)
{
    CHUGIN_SAFE_DELETE(b2BodyDef, b2_body_def_data_offset);
}

CK_DLL_MFUN(b2_body_def_set_type)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    def->type      = ckint_to_b2BodyType(GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(b2_body_def_set_position)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    t_CKVEC2 pos   = GET_NEXT_VEC2(ARGS);
    def->position  = { (float)pos.x, (float)pos.y };
}

CK_DLL_MFUN(b2_body_def_set_angle)
{
    B2_GET_BODY_DEF_PTR(SELF)->angle = GET_NEXT_FLOAT(ARGS);
}

CK_DLL_MFUN(b2_body_def_set_linear_velocity)
{
    t_CKVEC2 linear_velocity = GET_NEXT_VEC2(ARGS);
    B2_GET_BODY_DEF_PTR(SELF)->linearVelocity
      = { (float)linear_velocity.x, (float)linear_velocity.y };
}

CK_DLL_MFUN(b2_body_def_set_angular_velocity)
{
    B2_GET_BODY_DEF_PTR(SELF)->angularVelocity = GET_NEXT_FLOAT(ARGS);
}

CK_DLL_MFUN(b2_body_def_set_linear_damping)
{
    B2_GET_BODY_DEF_PTR(SELF)->linearDamping = GET_NEXT_FLOAT(ARGS);
}

CK_DLL_MFUN(b2_body_def_set_angular_damping)
{
    B2_GET_BODY_DEF_PTR(SELF)->angularDamping = GET_NEXT_FLOAT(ARGS);
}

CK_DLL_MFUN(b2_body_def_set_gravity_scale)
{
    B2_GET_BODY_DEF_PTR(SELF)->gravityScale = GET_NEXT_FLOAT(ARGS);
}

CK_DLL_MFUN(b2_body_def_set_sleep_threshold)
{
    B2_GET_BODY_DEF_PTR(SELF)->sleepThreshold = GET_NEXT_FLOAT(ARGS);
}

CK_DLL_MFUN(b2_body_def_set_enable_sleep)
{
    B2_GET_BODY_DEF_PTR(SELF)->enableSleep = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_body_def_set_awake)
{
    B2_GET_BODY_DEF_PTR(SELF)->isAwake = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_body_def_set_fixed_rotation)
{
    B2_GET_BODY_DEF_PTR(SELF)->fixedRotation = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_body_def_set_bullet)
{
    B2_GET_BODY_DEF_PTR(SELF)->isBullet = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_body_def_set_enabled)
{
    B2_GET_BODY_DEF_PTR(SELF)->isEnabled = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_body_def_set_automatic_mass)
{
    B2_GET_BODY_DEF_PTR(SELF)->automaticMass = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_body_def_get_type)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    RETURN->v_int  = def->type;
}

CK_DLL_MFUN(b2_body_def_get_position)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    RETURN->v_vec2 = b2Vec2_TO_VEC2(def->position);
}

CK_DLL_MFUN(b2_body_def_get_angle)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    RETURN->v_float = def->angle;
}

CK_DLL_MFUN(b2_body_def_get_linear_velocity)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    RETURN->v_vec2 = b2Vec2_TO_VEC2(def->linearVelocity);
}

CK_DLL_MFUN(b2_body_def_get_angular_velocity)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    RETURN->v_float = def->angularVelocity;
}

CK_DLL_MFUN(b2_body_def_get_linear_damping)
{
    RETURN->v_float = B2_GET_BODY_DEF_PTR(SELF)->linearDamping;
}

CK_DLL_MFUN(b2_body_def_get_angular_damping)
{
    RETURN->v_float = B2_GET_BODY_DEF_PTR(SELF)->angularDamping;
}

CK_DLL_MFUN(b2_body_def_get_gravity_scale)
{
    RETURN->v_float = B2_GET_BODY_DEF_PTR(SELF)->gravityScale;
}

CK_DLL_MFUN(b2_body_def_get_sleep_threshold)
{
    RETURN->v_float = B2_GET_BODY_DEF_PTR(SELF)->sleepThreshold;
}

CK_DLL_MFUN(b2_body_def_get_enable_sleep)
{
    RETURN->v_int = B2_GET_BODY_DEF_PTR(SELF)->enableSleep;
}

CK_DLL_MFUN(b2_body_def_get_awake)
{
    RETURN->v_int = B2_GET_BODY_DEF_PTR(SELF)->isAwake;
}

CK_DLL_MFUN(b2_body_def_get_fixed_rotation)
{
    RETURN->v_int = B2_GET_BODY_DEF_PTR(SELF)->fixedRotation;
}

CK_DLL_MFUN(b2_body_def_get_bullet)
{
    RETURN->v_int = B2_GET_BODY_DEF_PTR(SELF)->isBullet;
}

CK_DLL_MFUN(b2_body_def_get_enabled)
{
    RETURN->v_int = B2_GET_BODY_DEF_PTR(SELF)->isEnabled;
}

CK_DLL_MFUN(b2_body_def_get_automatic_mass)
{
    RETURN->v_int = B2_GET_BODY_DEF_PTR(SELF)->automaticMass;
}

// ============================================================================
// b2Body
// ============================================================================

CK_DLL_MFUN(b2_body_create_polygon_shape)
{
    Chuck_Object* shape_def_obj = GET_NEXT_OBJECT(ARGS);
    Chuck_Object* polygon       = GET_NEXT_OBJECT(ARGS);

    b2ShapeDef* shape_def
      = (b2ShapeDef*)OBJ_MEMBER_UINT(shape_def_obj, b2_shape_def_data_offset);
    b2Polygon* poly
      = (b2Polygon*)OBJ_MEMBER_UINT(polygon, b2_polygon_data_offset);
    b2BodyId body_id = *(b2BodyId*)OBJ_MEMBER_DATA(SELF, b2_body_data_offset);

    b2ShapeId shape_id = b2CreatePolygonShape(body_id, shape_def, poly);

    Chuck_Object* b2_shape = chugin_createCkObj("b2_Shape", false, SHRED);
    *(b2ShapeId*)OBJ_MEMBER_DATA(b2_shape, b2_shape_data_offset) = shape_id;

    RETURN->v_object = b2_shape;
}

CK_DLL_MFUN(b2_body_get_type)
{
    RETURN->v_int = b2Body_GetType(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_set_type)
{
    b2Body_SetType(B2_GET_BODY_ID(SELF),
                   ckint_to_b2BodyType(GET_NEXT_INT(ARGS)));
}

CK_DLL_MFUN(b2_body_get_position)
{
    b2Vec2 pos     = b2Body_GetPosition(B2_GET_BODY_ID(SELF));
    RETURN->v_vec2 = { pos.x, pos.y };
}

CK_DLL_MFUN(b2_body_get_rotation)
{
    b2Rot rot         = b2Body_GetRotation(B2_GET_BODY_ID(SELF));
    RETURN->v_complex = { rot.c, rot.s };
}

CK_DLL_MFUN(b2_body_get_angle)
{
    RETURN->v_float = b2Body_GetAngle(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_set_transform)
{
    t_CKVEC2 pos = GET_NEXT_VEC2(ARGS);
    float angle  = GET_NEXT_FLOAT(ARGS);
    b2Body_SetTransform(B2_GET_BODY_ID(SELF), { (float)pos.x, (float)pos.y },
                        angle);
}

CK_DLL_MFUN(b2_body_get_local_point)
{
    t_CKVEC2 world_point = GET_NEXT_VEC2(ARGS);
    b2Vec2 local_point   = b2Body_GetLocalPoint(
      B2_GET_BODY_ID(SELF), { (float)world_point.x, (float)world_point.y });
    RETURN->v_vec2 = { local_point.x, local_point.y };
}

CK_DLL_MFUN(b2_body_get_world_point)
{
    t_CKVEC2 local_point = GET_NEXT_VEC2(ARGS);
    b2Vec2 world_point   = b2Body_GetWorldPoint(
      B2_GET_BODY_ID(SELF), { (float)local_point.x, (float)local_point.y });
    RETURN->v_vec2 = { world_point.x, world_point.y };
}

CK_DLL_MFUN(b2_body_get_local_vector)
{
    t_CKVEC2 world_vector = GET_NEXT_VEC2(ARGS);
    b2Vec2 local_vector   = b2Body_GetLocalVector(
      B2_GET_BODY_ID(SELF), { (float)world_vector.x, (float)world_vector.y });
    RETURN->v_vec2 = { local_vector.x, local_vector.y };
}

CK_DLL_MFUN(b2_body_get_world_vector)
{
    t_CKVEC2 local_vector = GET_NEXT_VEC2(ARGS);
    b2Vec2 world_vector   = b2Body_GetWorldVector(
      B2_GET_BODY_ID(SELF), { (float)local_vector.x, (float)local_vector.y });
    RETURN->v_vec2 = { world_vector.x, world_vector.y };
}

CK_DLL_MFUN(b2_body_get_linear_velocity)
{
    b2Vec2 vel     = b2Body_GetLinearVelocity(B2_GET_BODY_ID(SELF));
    RETURN->v_vec2 = { vel.x, vel.y };
}

CK_DLL_MFUN(b2_body_set_linear_velocity)
{
    t_CKVEC2 vel = GET_NEXT_VEC2(ARGS);
    b2Body_SetLinearVelocity(B2_GET_BODY_ID(SELF),
                             { (float)vel.x, (float)vel.y });
}

CK_DLL_MFUN(b2_body_get_angular_velocity)
{
    RETURN->v_float = b2Body_GetAngularVelocity(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_set_angular_velocity)
{
    b2Body_SetAngularVelocity(B2_GET_BODY_ID(SELF), GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(b2_body_apply_force)
{
    t_CKVEC2 force = GET_NEXT_VEC2(ARGS);
    t_CKVEC2 point = GET_NEXT_VEC2(ARGS);
    t_CKINT wake   = GET_NEXT_INT(ARGS);
    b2Body_ApplyForce(B2_GET_BODY_ID(SELF), { (float)force.x, (float)force.y },
                      { (float)point.x, (float)point.y }, wake);
}

CK_DLL_MFUN(b2_body_apply_force_to_center)
{
    t_CKVEC2 force = GET_NEXT_VEC2(ARGS);
    t_CKINT wake   = GET_NEXT_INT(ARGS);
    b2Body_ApplyForceToCenter(B2_GET_BODY_ID(SELF),
                              { (float)force.x, (float)force.y }, wake);
}

CK_DLL_MFUN(b2_body_apply_torque)
{
    t_CKFLOAT torque = GET_NEXT_FLOAT(ARGS);
    t_CKINT wake     = GET_NEXT_INT(ARGS);
    b2Body_ApplyTorque(B2_GET_BODY_ID(SELF), torque, wake);
}

CK_DLL_MFUN(b2_body_apply_linear_impulse)
{
    t_CKVEC2 impulse = GET_NEXT_VEC2(ARGS);
    t_CKVEC2 point   = GET_NEXT_VEC2(ARGS);
    t_CKINT wake     = GET_NEXT_INT(ARGS);
    b2Body_ApplyLinearImpulse(B2_GET_BODY_ID(SELF),
                              { (float)impulse.x, (float)impulse.y },
                              { (float)point.x, (float)point.y }, wake);
}

CK_DLL_MFUN(b2_body_apply_linear_impulse_to_center)
{
    t_CKVEC2 impulse = GET_NEXT_VEC2(ARGS);
    t_CKINT wake     = GET_NEXT_INT(ARGS);
    b2Body_ApplyLinearImpulseToCenter(
      B2_GET_BODY_ID(SELF), { (float)impulse.x, (float)impulse.y }, wake);
}

CK_DLL_MFUN(b2_body_apply_angular_impulse)
{
    t_CKFLOAT impulse = GET_NEXT_FLOAT(ARGS);
    t_CKINT wake      = GET_NEXT_INT(ARGS);
    b2Body_ApplyAngularImpulse(B2_GET_BODY_ID(SELF), impulse, wake);
}

CK_DLL_MFUN(b2_body_get_mass)
{
    RETURN->v_float = b2Body_GetMass(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_get_inertia)
{
    RETURN->v_float = b2Body_GetInertiaTensor(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_get_local_center_of_mass)
{
    b2Vec2 center  = b2Body_GetLocalCenterOfMass(B2_GET_BODY_ID(SELF));
    RETURN->v_vec2 = { center.x, center.y };
}

CK_DLL_MFUN(b2_body_get_world_center_of_mass)
{
    b2Vec2 center  = b2Body_GetWorldCenterOfMass(B2_GET_BODY_ID(SELF));
    RETURN->v_vec2 = { center.x, center.y };
}

CK_DLL_MFUN(b2_body_get_mass_data)
{
    b2MassData data           = b2Body_GetMassData(B2_GET_BODY_ID(SELF));
    Chuck_Object* mass_data   = chugin_createCkObj("b2_MassData", false, SHRED);
    b2MassData* mass_data_ptr = new b2MassData;

    OBJ_MEMBER_UINT(mass_data, b2_mass_data_offset) = (t_CKUINT)mass_data_ptr;

    // TODO: impl after Ge gets back on API->object->create() not invoking
    // default ctor

    memcpy(mass_data_ptr, &data, sizeof(data));

    RETURN->v_object = mass_data;
}

CK_DLL_MFUN(b2_body_set_mass_data)
{
    Chuck_Object* mass_data_obj = GET_NEXT_OBJECT(ARGS);
    b2MassData* mass_data
      = (b2MassData*)OBJ_MEMBER_UINT(mass_data_obj, b2_mass_data_offset);
    b2Body_SetMassData(B2_GET_BODY_ID(SELF), *mass_data);
}

CK_DLL_MFUN(b2_body_apply_mass_from_shapes)
{
    b2Body_ApplyMassFromShapes(B2_GET_BODY_ID(SELF));
}

// CK_DLL_MFUN(b2_body_set_automatic_mass)
// {
//     b2Body_SetAutomaticMass(B2_GET_BODY_ID(SELF), GET_NEXT_INT(ARGS));
// }

// CK_DLL_MFUN(b2_body_get_automatic_mass)
// {
//     RETURN->v_int = b2Body_GetAutomaticMass(B2_GET_BODY_ID(SELF));
// }

CK_DLL_MFUN(b2_body_set_linear_damping)
{
    b2Body_SetLinearDamping(B2_GET_BODY_ID(SELF), GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(b2_body_get_linear_damping)
{
    RETURN->v_float = b2Body_GetLinearDamping(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_set_angular_damping)
{
    b2Body_SetAngularDamping(B2_GET_BODY_ID(SELF), GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(b2_body_get_angular_damping)
{
    RETURN->v_float = b2Body_GetAngularDamping(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_set_gravity_scale)
{
    b2Body_SetGravityScale(B2_GET_BODY_ID(SELF), GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(b2_body_get_gravity_scale)
{
    RETURN->v_float = b2Body_GetGravityScale(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_is_awake)
{
    RETURN->v_int = b2Body_IsAwake(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_set_awake)
{
    b2Body_SetAwake(B2_GET_BODY_ID(SELF), GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(b2_body_enable_sleep)
{
    b2Body_EnableSleep(B2_GET_BODY_ID(SELF), GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(b2_body_is_sleep_enabled)
{
    RETURN->v_int = b2Body_IsSleepEnabled(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_set_sleep_threshold)
{
    b2Body_SetSleepThreshold(B2_GET_BODY_ID(SELF), GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(b2_body_get_sleep_threshold)
{
    RETURN->v_float = b2Body_GetSleepThreshold(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_is_enabled)
{
    RETURN->v_int = b2Body_IsEnabled(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_disable)
{
    b2Body_Disable(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_enable)
{
    b2Body_Enable(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_set_fixed_rotation)
{
    b2Body_SetFixedRotation(B2_GET_BODY_ID(SELF), GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(b2_body_is_fixed_rotation)
{
    RETURN->v_int = b2Body_IsFixedRotation(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_set_bullet)
{
    b2Body_SetBullet(B2_GET_BODY_ID(SELF), GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(b2_body_is_bullet)
{
    RETURN->v_int = b2Body_IsBullet(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_enable_hit_events)
{
    b2Body_EnableHitEvents(B2_GET_BODY_ID(SELF), GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(b2_body_get_shape_count)
{
    RETURN->v_int = b2Body_GetShapeCount(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_get_shapes)
{
    // TODO
}

CK_DLL_MFUN(b2_body_get_joint_count)
{
    RETURN->v_int = b2Body_GetJointCount(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_get_joints)
{
    // TODO
}

CK_DLL_MFUN(b2_body_get_contact_capacity)
{
    RETURN->v_int = b2Body_GetContactCapacity(B2_GET_BODY_ID(SELF));
}

CK_DLL_MFUN(b2_body_get_contact_data)
{
    // TODO
}

CK_DLL_MFUN(b2_body_compute_aabb)
{
    b2AABB box     = b2Body_ComputeAABB(B2_GET_BODY_ID(SELF));
    RETURN->v_vec4 = { box.lowerBound.x, box.lowerBound.y, box.upperBound.x,
                       box.upperBound.y };
}

// ============================================================================
// b2Filter
// ============================================================================

CK_DLL_CTOR(b2_Filter_ctor)
{
    b2Filter* filter = B2_GET_FILTER_PTR(SELF);
    *filter          = b2DefaultFilter();
}

CK_DLL_MFUN(b2_Filter_get_categoryBits)
{
    b2Filter* filter = B2_GET_FILTER_PTR(SELF);
    RETURN->v_int    = filter->categoryBits;
}

CK_DLL_MFUN(b2_Filter_get_maskBits)
{
    b2Filter* filter = B2_GET_FILTER_PTR(SELF);
    RETURN->v_int    = filter->maskBits;
}

CK_DLL_MFUN(b2_Filter_get_groupIndex)
{
    b2Filter* filter = B2_GET_FILTER_PTR(SELF);
    RETURN->v_int    = filter->groupIndex;
}

CK_DLL_MFUN(b2_Filter_set_categoryBits)
{
    b2Filter* filter     = B2_GET_FILTER_PTR(SELF);
    filter->categoryBits = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_Filter_set_maskBits)
{
    b2Filter* filter = B2_GET_FILTER_PTR(SELF);
    filter->maskBits = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(b2_Filter_set_groupIndex)
{
    b2Filter* filter   = B2_GET_FILTER_PTR(SELF);
    filter->groupIndex = GET_NEXT_INT(ARGS);
}

// ============================================================================
// b2World
// ============================================================================

CK_DLL_CTOR(b2_world_ctor)
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    (*(b2WorldId*)OBJ_MEMBER_DATA(SELF, b2_world_data_offset))
      = b2CreateWorld(&worldDef);
}

CK_DLL_MFUN(b2_world_create_body)
{
    b2WorldId world_id
      = *(b2WorldId*)OBJ_MEMBER_DATA(SELF, b2_world_data_offset);

    Chuck_Object* body_def_obj = GET_NEXT_OBJECT(ARGS);
    b2BodyDef* bodyDef
      = (b2BodyDef*)OBJ_MEMBER_UINT(body_def_obj, b2_body_def_data_offset);

    // b2BodyId body_id = b2CreateBody(world_id, bodyDef);
    b2BodyId body_id = b2CreateBody(world_id, bodyDef);

    b2Vec2 position = b2Body_GetPosition(body_id);
    float angle     = b2Body_GetAngle(body_id);
    printf("%4.2f %4.2f %4.2f\n", position.x, position.y, angle);

    Chuck_Object* b2_body = chugin_createCkObj("b2_Body", false, SHRED);
    *(b2BodyId*)OBJ_MEMBER_DATA(b2_body, b2_body_data_offset) = body_id;

    RETURN->v_object = b2_body;
}

void ulib_box2d_query(Chuck_DL_Query* QUERY)
{

    // b2_Filter --------------------------------------
    BEGIN_CLASS("b2_Filter", "Object");
    DOC_CLASS(
      "This is used to filter collision on shapes. It affects shape-vs-shape "
      "collision"
      "and shape-versus-query collision (such as b2World_CastRay).");
    b2_filter_data_offset
      = MVAR("vec3", "@b2_filter_category_bits_offset", false);
    CTOR(b2_Filter_ctor);
    MFUN(b2_Filter_get_categoryBits, "int", "categoryBits");
    DOC_FUNC(
      "The collision category bits. Normally you would just set one bit."
      "The category bits should represent your application object types."
      "For example:"
      "static = 0x00000001, dynamic = 0x00000002, debris = 0x00000004, player "
      "= 0x00000008, etc");
    MFUN(b2_Filter_get_maskBits, "int", "maskBits");
    DOC_FUNC(
      "The collision mask bits. This states the categories that this"
      "shape would accept for collision."
      "For example, you may want your player to only collide with static "
      "objects"
      "and other players. e.g. maskBits = Static | Player;");

    MFUN(b2_Filter_get_groupIndex, "int", "groupIndex");
    DOC_FUNC(
      "Collision groups allow a certain group of objects to never collide "
      "(negative)"
      "or always collide (positive). A group index of zero has no effect. "
      "Non-zero group filtering"
      "always wins against the mask bits."
      "For example, you may want ragdolls to collide with other ragdolls but "
      "you don't want"
      "ragdoll self-collision. In this case you would give each ragdoll a "
      "unique negative group index"
      "and apply that group index to all shapes on the ragdoll.");

    MFUN(b2_Filter_set_categoryBits, "void", "categoryBits");
    ARG("int", "categoryBits");

    MFUN(b2_Filter_set_maskBits, "void", "maskBits");
    ARG("int", "maskBits");

    MFUN(b2_Filter_set_groupIndex, "void", "groupIndex");
    ARG("int", "groupIndex");

    END_CLASS(); // b2_Filter

    // b2_ShapeDef --------------------------------------
    BEGIN_CLASS("b2_ShapeDef", "Object");
    DOC_CLASS(
      "Used to create a shape."
      "This is a temporary object used to bundle shape creation parameters. "
      "You may use"
      "the same shape definition to create multiple shapes.");
    b2_shape_def_data_offset = MVAR("int", "@b2_shape_def_data_offset", false);
    CTOR(b2_ShapeDef_ctor);
    DTOR(b2_ShapeDef_dtor);

    // getters
    MFUN(b2_ShapeDef_get_friction, "float", "friction");
    DOC_FUNC(
      "The Coulomb (dry) friction coefficient, usually in the range [0,1].");

    MFUN(b2_ShapeDef_get_restitution, "float", "restitution");
    DOC_FUNC("The restitution (bounce) usually in the range [0,1].");

    MFUN(b2_ShapeDef_get_density, "float", "density");
    DOC_FUNC("The density, usually in kg/m^2.");

    MFUN(b2_ShapeDef_get_isSensor, "int", "isSensor");
    DOC_FUNC(
      "A sensor shape generates overlap events but never generates a collision "
      "response.");

    MFUN(b2_ShapeDef_get_enableSensorEvents, "int", "enableSensorEvents");
    DOC_FUNC(
      "Enable sensor events for this shape. Only applies to kinematic and "
      "dynamic bodies. Ignored for sensors.");

    MFUN(b2_ShapeDef_get_enableContactEvents, "int", "enableContactEvents");
    DOC_FUNC(
      "Enable contact events for this shape. Only applies to kinematic and "
      "dynamic bodies. Ignored for sensors.");

    MFUN(b2_ShapeDef_get_enableHitEvents, "int", "enableHitEvents");
    DOC_FUNC(
      "Enable hit events for this shape. Only applies to kinematic and dynamic "
      "bodies. Ignored for sensors.");

    MFUN(b2_ShapeDef_get_enablePreSolveEvents, "int", "enablePreSolveEvents");
    DOC_FUNC(
      "Enable pre-solve contact events for this shape. Only applies to dynamic "
      "bodies. These are expensive and must be carefully handled due to "
      "threading. Ignored for sensors.");

    MFUN(b2_ShapeDef_get_forceContactCreation, "int", "forceContactCreation");
    DOC_FUNC(
      "Normally shapes on static bodies don't invoke contact creation when "
      "they are added to the world. This overrides that behavior and causes "
      "contact creation. This significantly slows down static body creation "
      "which can be important when there are many static shapes.");

    MFUN(b2_ShapeDef_get_filter, "b2_Filter", "filter");
    DOC_FUNC("The collision filter data.");

    // setters
    MFUN(b2_ShapeDef_set_friction, "void", "friction");
    ARG("float", "friction");
    DOC_FUNC(
      "The Coulomb (dry) friction coefficient, usually in the range [0,1].");

    MFUN(b2_ShapeDef_set_restitution, "void", "restitution");
    ARG("float", "restitution");
    DOC_FUNC("The restitution (bounce) usually in the range [0,1].");

    MFUN(b2_ShapeDef_set_density, "void", "density");
    ARG("float", "density");
    DOC_FUNC("The density, usually in kg/m^2.");

    MFUN(b2_ShapeDef_set_isSensor, "void", "isSensor");
    ARG("int", "isSensor");
    DOC_FUNC(
      "A sensor shape generates overlap events but never generates a collision "
      "response.");

    MFUN(b2_ShapeDef_set_enableSensorEvents, "void", "enableSensorEvents");
    ARG("int", "enableSensorEvents");
    DOC_FUNC(
      "Enable sensor events for this shape. Only applies to kinematic and "
      "dynamic bodies. Ignored for sensors.");

    MFUN(b2_ShapeDef_set_enableContactEvents, "void", "enableContactEvents");
    ARG("int", "enableContactEvents");
    DOC_FUNC(
      "Enable contact events for this shape. Only applies to kinematic and "
      "dynamic bodies. Ignored for sensors.");

    MFUN(b2_ShapeDef_set_enableHitEvents, "void", "enableHitEvents");
    ARG("int", "enableHitEvents");
    DOC_FUNC(
      "Enable hit events for this shape. Only applies to kinematic and dynamic "
      "bodies. Ignored for sensors.");

    MFUN(b2_ShapeDef_set_enablePreSolveEvents, "void", "enablePreSolveEvents");
    ARG("int", "enablePreSolveEvents");
    DOC_FUNC(
      "Enable pre-solve contact events for this shape. Only applies to dynamic "
      "bodies. These are expensive and must be carefully handled due to "
      "threading. Ignored for sensors.");

    MFUN(b2_ShapeDef_set_forceContactCreation, "void", "forceContactCreation");
    ARG("int", "forceContactCreation");
    DOC_FUNC(
      "Normally shapes on static bodies don't invoke contact creation when "
      "they are added to the world. This overrides that behavior and causes "
      "contact creation. This significantly slows down static body creation "
      "which can be important when there are many static shapes.");

    MFUN(b2_ShapeDef_set_filter, "void", "filter");
    ARG("b2_Filter", "filter");

    END_CLASS(); // b2_ShapeDef

    // b2_MassData --------------------------------------
    BEGIN_CLASS("b2_MassData", "Object");
    b2_mass_data_offset = MVAR("int", "@b2_mass_data_offset", false);
    CTOR(b2_MassData_ctor);
    DTOR(b2_MassData_dtor);

    // getters
    MFUN(b2_MassData_get_mass, "float", "mass");
    DOC_FUNC("The mass of the shape, usually in kilograms.");

    MFUN(b2_MassData_get_center, "vec2", "center");
    DOC_FUNC(
      "The position of the shape's centroid relative to the shape's origin.");

    MFUN(b2_MassData_get_I, "float", "I");
    DOC_FUNC("The rotational inertia of the shape about the local origin.");

    // setters
    MFUN(b2_MassData_set_mass, "void", "mass");
    ARG("float", "mass");
    DOC_FUNC("The mass of the shape, usually in kilograms.");

    MFUN(b2_MassData_set_center, "void", "center");
    ARG("vec2", "center");
    DOC_FUNC(
      "The position of the shape's centroid relative to the shape's origin.");

    MFUN(b2_MassData_set_I, "void", "I");
    ARG("float", "I");
    DOC_FUNC("The rotational inertia of the shape about the local origin.");

    END_CLASS(); // b2_MassData

    // b2_BodyType --------------------------------------

    BEGIN_CLASS("b2_BodyType", "Object");
    static t_CKINT b2_static_body    = 0;
    static t_CKINT b2_kinematic_body = 1;
    static t_CKINT b2_dynamic_body   = 2;
    SVAR("int", "staticBody", &b2_static_body);
    DOC_VAR("zero mass, zero velocity, may be manually moved");
    SVAR("int", "kinematicBody", &b2_kinematic_body);
    DOC_VAR("zero mass, velocity set by user, moved by solver");
    SVAR("int", "dynamicBody", &b2_dynamic_body);
    DOC_VAR("positive mass, velocity determined by forces, moved by solver");
    END_CLASS();

    BEGIN_CLASS("b2_WorldDef", "Object");
    b2_world_def_data_offset = MVAR("int", "@b2_world_def_data", false);
    CTOR(b2_WorldDef_ctor);
    DTOR(b2_WorldDef_dtor);
    END_CLASS();

    BEGIN_CLASS("b2_BodyDef", "Object");
    CTOR(b2_body_def_ctor);
    DTOR(b2_body_def_dtor);

    MFUN(b2_body_def_get_type, "int", "type");
    DOC_FUNC("The body type: static, kinematic, or dynamic");

    MFUN(b2_body_def_set_type, "void", "type");
    ARG("int", "b2_BodyType");
    DOC_FUNC("The body type: static, kinematic, or dynamic");

    MFUN(b2_body_def_get_position, "vec2", "position");
    DOC_FUNC("Get the world position of the body's origin.");

    MFUN(b2_body_def_set_position, "void", "position");
    ARG("vec2", "position");
    DOC_FUNC(
      "The initial world position of the body. Bodies should be created with "
      "the desired position."
      "@note Creating bodies at the origin and then moving them nearly doubles "
      "the cost of body creation, especially"
      "if the body is moved after shapes have been added.");

    MFUN(b2_body_def_get_angle, "float", "angle");
    DOC_FUNC("Get the world angle of the body in radians.");

    MFUN(b2_body_def_set_angle, "void", "angle");
    ARG("float", "angle");
    DOC_FUNC("The initial world angle of the body in radians.");

    MFUN(b2_body_def_get_linear_velocity, "vec2", "linearVelocity");

    MFUN(b2_body_def_set_linear_velocity, "void", "linearVelocity");
    ARG("vec2", "linearVelocity");
    DOC_FUNC(
      "The initial linear velocity of the body's origin. Typically in meters "
      "per second.");

    MFUN(b2_body_def_get_angular_velocity, "float", "angularVelocity");

    MFUN(b2_body_def_set_angular_velocity, "void", "angularVelocity");
    ARG("float", "angularVelocity");
    DOC_FUNC("The initial angular velocity of the body in radians per second.");

    MFUN(b2_body_def_get_linear_damping, "float", "linearDamping");

    MFUN(b2_body_def_set_linear_damping, "void", "linearDamping");
    ARG("float", "linearDamping");
    DOC_FUNC(
      "Linear damping is use to reduce the linear velocity. The damping "
      "parameter"
      "can be larger than 1 but the damping effect becomes sensitive to the"
      "time step when the damping parameter is large."
      "Generally linear damping is undesirable because it makes objects move "
      "slowly"
      "as if they are floating.");

    MFUN(b2_body_def_get_angular_damping, "float", "angularDamping");

    MFUN(b2_body_def_set_angular_damping, "void", "angularDamping");
    ARG("float", "angularDamping");
    DOC_FUNC(
      "Angular damping is use to reduce the angular velocity. The damping "
      "parameter"
      "can be larger than 1.0f but the damping effect becomes sensitive to the"
      "time step when the damping parameter is large."
      "Angular damping can be use slow down rotating bodies.");

    MFUN(b2_body_def_get_gravity_scale, "float", "gravityScale");

    MFUN(b2_body_def_set_gravity_scale, "void", "gravityScale");
    ARG("float", "gravityScale");
    DOC_FUNC("Scale the gravity applied to this body.");

    MFUN(b2_body_def_get_sleep_threshold, "float", "sleepThreshold");

    MFUN(b2_body_def_set_sleep_threshold, "void", "sleepThreshold");
    ARG("float", "sleepThreshold");
    DOC_FUNC("Sleep velocity threshold, default is 0.05 meter per second");

    MFUN(b2_body_def_get_enable_sleep, "int", "enableSleep");

    MFUN(b2_body_def_set_enable_sleep, "void", "enableSleep");
    ARG("int", "enableSleep");
    DOC_FUNC("Set this flag to false if this body should never fall asleep.");

    MFUN(b2_body_def_get_awake, "int", "isAwake");

    MFUN(b2_body_def_set_awake, "void", "isAwake");
    ARG("int", "awake");
    DOC_FUNC("Is this body initially awake or sleeping?");

    MFUN(b2_body_def_get_fixed_rotation, "int", "fixedRotation");

    MFUN(b2_body_def_set_fixed_rotation, "void", "fixedRotation");
    ARG("int", "fixedRotation");
    DOC_FUNC(
      "Should this body be prevented from rotating? Useful for characters.");

    MFUN(b2_body_def_get_bullet, "int", "isBullet");

    MFUN(b2_body_def_set_bullet, "void", "isBullet");
    ARG("int", "bullet");
    DOC_FUNC(
      "Treat this body as high speed object that performs continuous collision "
      "detection"
      "against dynamic and kinematic bodies, but not other bullet bodies."
      "@warning Bullets should be used sparingly. They are not a solution for "
      "general dynamic-versus-dynamic"
      "continuous collision. They may interfere with joint constraints.");

    MFUN(b2_body_def_get_enabled, "int", "isEnabled");

    MFUN(b2_body_def_set_enabled, "void", "isEnabled");
    ARG("int", "enabled");
    DOC_FUNC(
      "Used to disable a body. A disabled body does not move or collide.");

    MFUN(b2_body_def_get_automatic_mass, "int", "automaticMass");

    MFUN(b2_body_def_set_automatic_mass, "void", "automaticMass");
    ARG("int", "automaticMass");
    DOC_FUNC(
      "Automatically compute mass and related properties on this body from "
      "shapes."
      "Triggers whenever a shape is add/removed/changed. Default is true.");
    END_CLASS();

    BEGIN_CLASS("b2_Polygon", "Object");
    DOC_CLASS(
      "A solid convex polygon. It is assumed that the interior of the polygon "
      "is to the left of each edge."
      "Polygons have a maximum number of vertices equal to "
      "b2_maxPolygonVertices."
      "In most cases you should not need many vertices for a convex polygon."
      "@warning DO NOT fill this out manually, instead use a helper function "
      "like b2MakePolygon or b2MakeBox.");

    b2_polygon_data_offset = MVAR("int", "@b2_polygon_data", false);

    DTOR(b2_polygon_dtor);

    // helpers
    SFUN(b2_polygon_make_box, "b2_Polygon", "makeBox");
    ARG("float", "hx");
    ARG("float", "hy");
    DOC_FUNC(
      "Make a box (rectangle) polygon, bypassing the need for a convex hull.");

    END_CLASS(); // b2_Polygon

    BEGIN_CLASS("b2_Shape", "Object");
    b2_shape_data_offset = MVAR("int", "@b2_shape_data", false);
    END_CLASS(); // b2_Shape

    // b2_Body -------------------------------------------------------
    BEGIN_CLASS("b2_Body", "Object");
    DOC_CLASS("Don't create bodies directly. Use b2_World.createBody instead.");
    b2_body_data_offset = MVAR("int", "@b2_body_data", false);

    // TODO: either refcount or manual memory management
    /// Destroy a rigid body given an id. This destroys all shapes and joints
    /// attached to the body.
    ///	Do not keep references to the associated shapes and joints.
    // B2_API void b2DestroyBody(b2BodyId bodyId);

    MFUN(b2_body_get_type, "int", "type");
    DOC_FUNC("Get the body type: static, kinematic, or dynamic");

    MFUN(b2_body_set_type, "void", "type");
    ARG("int", "b2_BodyType");
    DOC_FUNC(
      " Change the body type. This is an expensive operation. This "
      "automatically updates the mass properties regardless of the automatic "
      "mass setting.");

    MFUN(b2_body_get_position, "vec2", "position");
    DOC_FUNC(
      "Get the world position of a body. This is the location of the body "
      "origin.");

    MFUN(b2_body_get_rotation, "complex", "rotation");
    DOC_FUNC(
      "Get the world rotation of a body as a cosine/sine pair (complex "
      "number)");

    MFUN(b2_body_get_angle, "float", "angle");
    DOC_FUNC(" Get the body angle in radians in the range [-pi, pi]");

    MFUN(b2_body_set_transform, "void", "transform");
    ARG("vec2", "position");
    ARG("float", "angle");
    DOC_FUNC(
      "Set the world transform of a body. This acts as a teleport and is "
      "fairly expensive."
      "@note Generally you should create a body with then intended "
      "transform."
      "@see b2_BodyDef.position and b2_BodyDef.angle");

    MFUN(b2_body_get_local_point, "vec2", "localPoint");
    ARG("vec2", "worldPoint");
    DOC_FUNC("Get a local point on a body given a world point");

    MFUN(b2_body_get_world_point, "vec2", "worldPoint");
    ARG("vec2", "localPoint");
    DOC_FUNC("Get a world point on a body given a local point");

    MFUN(b2_body_get_local_vector, "vec2", "localVector");
    ARG("vec2", "worldVector");
    DOC_FUNC("Get a local vector on a body given a world vector");

    MFUN(b2_body_get_world_vector, "vec2", "worldVector");
    ARG("vec2", "localVector");
    DOC_FUNC("Get a world vector on a body given a local vector");

    MFUN(b2_body_get_linear_velocity, "vec2", "linearVelocity");
    DOC_FUNC(
      "Get the linear velocity of a body's center of mass. Typically in "
      "meters per second.");

    MFUN(b2_body_set_linear_velocity, "void", "linearVelocity");
    ARG("vec2", "linearVelocity");
    DOC_FUNC(
      "Set the linear velocity of a body. Typically in meters per second.");

    MFUN(b2_body_get_angular_velocity, "float", "angularVelocity");
    DOC_FUNC("Get the angular velocity of a body in radians per second");

    MFUN(b2_body_set_angular_velocity, "void", "angularVelocity");
    ARG("float", "angularVelocity");
    DOC_FUNC("Set the angular velocity of a body in radians per second");

    MFUN(b2_body_apply_force, "void", "force");
    ARG("vec2", "force");
    ARG("vec2", "point");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply a force at a world point. If the force is not applied at the "
      "center of mass, it will generate a torque and affect the angular "
      "velocity. This optionally wakes up the body."
      "The force is ignored if the body is not awake.");

    MFUN(b2_body_apply_force_to_center, "void", "force");
    ARG("vec2", "force");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply a force to the center of mass. This optionally wakes up the "
      "body. The force is ignored if the body is not awake.");

    MFUN(b2_body_apply_torque, "void", "torque");
    ARG("float", "torque");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply a torque. This affects the angular velocity without affecting "
      "the linear velocity."
      "This optionally wakes the body. The torque is ignored if the body "
      "is not awake.");

    MFUN(b2_body_apply_linear_impulse, "void", "impulse");
    ARG("vec2", "impulse");
    ARG("vec2", "point");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply an impulse at a point. This immediately modifies the velocity."
      "It also modifies the angular velocity if the point of application "
      "is not at the center of mass. This optionally wakes the body."
      "The impulse is ignored if the body is not awake."
      "@warning This should be used for one-shot impulses. If you need a "
      "steady force,"
      "use a force instead, which will work better with the sub-stepping "
      "solver.");

    MFUN(b2_body_apply_linear_impulse_to_center, "void", "impulse");
    ARG("vec2", "impulse");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply an impulse to the center of mass. This immediately modifies "
      "the velocity."
      "The impulse is ignored if the body is not awake. This optionally "
      "wakes the body."
      "@warning This should be used for one-shot impulses. If you need a "
      "steady force,"
      "use a force instead, which will work better with the sub-stepping "
      "solver.");

    MFUN(b2_body_apply_angular_impulse, "void", "impulse");
    ARG("float", "impulse");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply an angular impulse. The impulse is ignored if the body is not "
      "awake. This optionally wakes the body."
      "@warning This should be used for one-shot impulses. If you need a "
      "steady"
      "force, use a force instead, which will work better with the "
      "sub-stepping solver.");

    MFUN(b2_body_get_mass, "float", "mass");
    DOC_FUNC("Get the mass of the body, typically in kilograms");

    MFUN(b2_body_get_inertia, "float", "inertia");
    DOC_FUNC("Get the inertia tensor of the body, typically in kg*m^2");

    MFUN(b2_body_get_local_center_of_mass, "vec2", "localCenterOfMass");
    DOC_FUNC("Get the center of mass position of the body in local space");

    MFUN(b2_body_get_world_center_of_mass, "vec2", "worldCenterOfMass");
    DOC_FUNC("Get the center of mass position of the body in world space");

    MFUN(b2_body_get_mass_data, "b2_MassData", "massData");
    DOC_FUNC("Get the mass data for a body");

    MFUN(b2_body_set_mass_data, "void", "massData");
    ARG("b2_MassData", "massData");
    DOC_FUNC(
      "Override the body's mass properties. Normally this is computed "
      "automatically using the shape geometry and density."
      "This information is lost if a shape is added or removed or if the "
      "body type changes.");

    MFUN(b2_body_apply_mass_from_shapes, "void", "applyMassFromShapes");
    DOC_FUNC(
      "This update the mass properties to the sum of the mass properties "
      "of the shapes. This normally does not need to be called unless you "
      "called .massData(b2_MassData) to override the mass and you later want "
      "to reset "
      "the mass."
      "You may also use this when automatic mass computation has been "
      "disabled."
      "You should call this regardless of body type.");

    // automatic mass not yet added to box2c impl
    // MFUN(b2_body_set_automatic_mass, "void", "automaticMass");
    // ARG("int", "automaticMass");
    // DOC_FUNC(
    //   "Set the automatic mass setting. Normally this is set in b2BodyDef "
    //   "before creation.");

    // MFUN(b2_body_get_automatic_mass, "int", "automaticMass");
    // DOC_FUNC("Get the automatic mass setting");

    MFUN(b2_body_set_linear_damping, "void", "linearDamping");
    ARG("float", "linearDamping");
    DOC_FUNC(
      "Adjust the linear damping. Normally this is set in b2BodyDef before "
      "creation.");

    MFUN(b2_body_get_linear_damping, "float", "linearDamping");
    DOC_FUNC("Get the current linear damping.");

    MFUN(b2_body_set_angular_damping, "void", "angularDamping");
    ARG("float", "angularDamping");
    DOC_FUNC(
      "Adjust the angular damping. Normally this is set in b2BodyDef before "
      "creation.");

    MFUN(b2_body_get_angular_damping, "float", "angularDamping");
    DOC_FUNC("Get the current angular damping.");

    MFUN(b2_body_set_gravity_scale, "void", "gravityScale");
    ARG("float", "gravityScale");
    DOC_FUNC(
      "Adjust the gravity scale. Normally this is set in b2BodyDef before "
      "creation."
      "@see b2BodyDef::gravityScale");

    MFUN(b2_body_get_gravity_scale, "float", "gravityScale");
    DOC_FUNC("Get the current gravity scale");

    MFUN(b2_body_is_awake, "int", "isAwake");
    DOC_FUNC("Returns true if this body is awake");

    MFUN(b2_body_set_awake, "void", "awake");
    ARG("int", "awake");
    DOC_FUNC(
      "Wake a body from sleep. This wakes the entire island the body is "
      "touching."
      "@warning Putting a body to sleep will put the entire island of "
      "bodies"
      "touching this body to sleep, which can be expensive and possibly "
      "unintuitive.");

    MFUN(b2_body_enable_sleep, "void", "enableSleep");
    ARG("int", "enableSleep");
    DOC_FUNC(
      "Enable or disable sleeping for this body. If sleeping is disabled "
      "the body will wake.");

    MFUN(b2_body_is_sleep_enabled, "int", "isSleepEnabled");
    DOC_FUNC("Returns true if sleeping is enabled for this body");

    MFUN(b2_body_set_sleep_threshold, "void", "sleepThreshold");
    ARG("float", "sleepThreshold");
    DOC_FUNC("Set the sleep threshold, typically in meters per second");

    MFUN(b2_body_get_sleep_threshold, "float", "sleepThreshold");
    DOC_FUNC("Get the sleep threshold, typically in meters per second.");

    MFUN(b2_body_is_enabled, "int", "isEnabled");
    DOC_FUNC("Returns true if this body is enabled");

    MFUN(b2_body_disable, "void", "disable");
    DOC_FUNC(
      "Disable a body by removing it completely from the simulation. This "
      "is expensive.");

    MFUN(b2_body_enable, "void", "enable");
    DOC_FUNC(
      "Enable a body by adding it to the simulation. This is expensive.");

    MFUN(b2_body_set_fixed_rotation, "void", "fixedRotation");
    ARG("int", "flag");
    DOC_FUNC(
      "Set this body to have fixed rotation. This causes the mass to be "
      "reset in all cases.");

    MFUN(b2_body_is_fixed_rotation, "int", "isFixedRotation");
    DOC_FUNC("Does this body have fixed rotation?");

    MFUN(b2_body_set_bullet, "void", "bullet");
    ARG("int", "flag");
    DOC_FUNC(
      "Set this body to be a bullet. A bullet does continuous collision "
      "detection against dynamic bodies (but not other bullets).");

    MFUN(b2_body_is_bullet, "int", "isBullet");
    DOC_FUNC("Is this body a bullet?");

    MFUN(b2_body_enable_hit_events, "void", "enableHitEvents");
    ARG("int", "enableHitEvents");
    DOC_FUNC(
      "Enable/disable hit events on all shapes"
      "@see b2ShapeDef::enableHitEvents");

    MFUN(b2_body_get_shape_count, "int", "shapeCount");
    DOC_FUNC("Get the number of shapes on this body");

    // MFUN(b2_body_get_shapes, "b2_Shape[]", "shapes");
    // DOC_FUNC("Get the shape ids for all shapes on this body");

    MFUN(b2_body_get_joint_count, "int", "jointCount");
    DOC_FUNC("Get the number of joints on this body");

    // MFUN(b2_body_get_joints, "b2_Joint[]", "joints");
    // DOC_FUNC("Get the joint ids for all joints on this body");

    MFUN(b2_body_get_contact_capacity, "int", "contactCapacity");
    DOC_FUNC(
      "Get the maximum capacity required for retrieving all the touching "
      "contacts on a body");

    // MFUN(b2_body_get_contact_data, "b2_ContactData[]", "contactData");
    // DOC_FUNC("Get the touching contact data for a body");

    MFUN(b2_body_compute_aabb, "vec4", "computeAABB");
    DOC_FUNC(
      "Get the current world AABB that contains all the attached shapes. Note "
      "that this may not encompass the body origin."
      "If there are no shapes attached then the returned AABB is empty and "
      "centered on the body origin."
      "The aabb is in the form of (lowerBound.x, lowerBound.y, upperBound.x, "
      "upperBound.y");

    // shape creation

    MFUN(b2_body_create_polygon_shape, "b2_Shape", "createPolygonShape");
    ARG("b2_ShapeDef", "shapeDef");
    ARG("b2_Polygon", "polygon");
    DOC_FUNC(
      "Create a polygon shape and attach it to a body. The shape definition and"
      "geometry are fully cloned. Contacts are not created until the next time"
      "step."
      "@return the shape id for accessing the shape");

    END_CLASS(); // b2_Body

    // b2_World --------------------------------------
    BEGIN_CLASS("b2_World", "Object");
    b2_world_data_offset = MVAR("int", "@b2_world_data", false);
    CTOR(b2_world_ctor);

    MFUN(b2_world_create_body, "b2_Body", "createBody");
    ARG("b2_BodyDef", "bodyDef");

    END_CLASS(); // b2_World
}
#include <box2d/box2d.h>
#include <chuck/chugin.h>

#include "ulib_helper.h"

static_assert(sizeof(void*) == sizeof(t_CKUINT), "pointer size mismatch");
// make sure b2 ids fit within a chuck int
static_assert(sizeof(b2WorldId) <= sizeof(t_CKINT), "b2Worldsize mismatch");
static_assert(sizeof(b2BodyId) <= sizeof(t_CKINT), "b2Body size mismatch");
static_assert(sizeof(b2ShapeId) <= sizeof(t_CKINT), "b2Shape size mismatch");

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
static t_CKUINT b2_world_data_offset     = 0;
static t_CKUINT b2_body_data_offset      = 0;
static t_CKUINT b2_shape_data_offset     = 0;
static t_CKUINT b2_polygon_data_offset   = 0;

// TODO: macros for setting/getting b2 ids
// need to differentiate between b2Ids that are stored directly IN chuck object
// vs larger structs that are stored as a pointer to heap memory

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
      = (b2BodyDef*)OBJ_MEMBER_DATA(body_def_obj, b2_body_def_data_offset);
    UNUSED_VAR(bodyDef);

    b2BodyDef default_body_def = b2DefaultBodyDef();

    // b2BodyId body_id = b2CreateBody(world_id, bodyDef);
    b2BodyId body_id = b2CreateBody(world_id, &default_body_def);

    Chuck_Object* b2_body = chugin_createCkObj("b2_Body", false, SHRED);
    *(b2BodyId*)OBJ_MEMBER_DATA(b2_body, b2_body_data_offset) = body_id;

    RETURN->v_object = b2_body;
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

CK_DLL_MFUN(b2_body_set_type)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    def->type      = ckint_to_b2BodyType(GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(b2_body_set_position)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    t_CKVEC2 pos   = GET_NEXT_VEC2(ARGS);
    def->position  = { (float)pos.x, (float)pos.y };
}

CK_DLL_MFUN(b2_body_set_angle)
{
}

CK_DLL_MFUN(b2_body_set_linear_velocity)
{
}

CK_DLL_MFUN(b2_body_set_angular_velocity)
{
}

CK_DLL_MFUN(b2_body_set_linear_damping)
{
}

CK_DLL_MFUN(b2_body_set_angular_damping)
{
}

CK_DLL_MFUN(b2_body_set_gravity_scale)
{
}

CK_DLL_MFUN(b2_body_set_sleep_threshold)
{
}

CK_DLL_MFUN(b2_body_set_enable_sleep)
{
}

CK_DLL_MFUN(b2_body_set_awake)
{
}

CK_DLL_MFUN(b2_body_set_fixed_rotation)
{
}

CK_DLL_MFUN(b2_body_set_bullet)
{
}

CK_DLL_MFUN(b2_body_set_enabled)
{
}

CK_DLL_MFUN(b2_body_set_automatic_mass)
{
}

CK_DLL_MFUN(b2_body_get_type)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    RETURN->v_int  = def->type;
}

CK_DLL_MFUN(b2_body_get_position)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    RETURN->v_vec2 = b2Vec2_TO_VEC2(def->position);
}

CK_DLL_MFUN(b2_body_get_angle)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    RETURN->v_float = def->angle;
}

CK_DLL_MFUN(b2_body_get_linear_velocity)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    RETURN->v_vec2 = b2Vec2_TO_VEC2(def->linearVelocity);
}

CK_DLL_MFUN(b2_body_get_angular_velocity)
{
    b2BodyDef* def = (b2BodyDef*)OBJ_MEMBER_UINT(SELF, b2_body_def_data_offset);
    RETURN->v_float = def->angularVelocity;
}

CK_DLL_MFUN(b2_body_get_linear_damping)
{
}

CK_DLL_MFUN(b2_body_get_angular_damping)
{
}

CK_DLL_MFUN(b2_body_get_gravity_scale)
{
}

CK_DLL_MFUN(b2_body_get_sleep_threshold)
{
}

CK_DLL_MFUN(b2_body_get_enable_sleep)
{
}

CK_DLL_MFUN(b2_body_get_awake)
{
}

CK_DLL_MFUN(b2_body_get_fixed_rotation)
{
}

CK_DLL_MFUN(b2_body_get_bullet)
{
}

CK_DLL_MFUN(b2_body_get_enabled)
{
}

CK_DLL_MFUN(b2_body_get_automatic_mass)
{
}

// ============================================================================
// b2BodyType
// ============================================================================

// ============================================================================
// b2Body
// ============================================================================

CK_DLL_MFUN(b2_body_create_polygon_shape)
{
    Chuck_Object* shape_def = GET_NEXT_OBJECT(ARGS);
    UNUSED_VAR(shape_def);
    Chuck_Object* polygon = GET_NEXT_OBJECT(ARGS);

    b2ShapeDef defaultShapeDef = b2DefaultShapeDef();
    b2Polygon* poly
      = (b2Polygon*)OBJ_MEMBER_UINT(polygon, b2_polygon_data_offset);
    b2BodyId body_id = *(b2BodyId*)OBJ_MEMBER_DATA(SELF, b2_body_data_offset);

    b2ShapeId shape_id = b2CreatePolygonShape(body_id, &defaultShapeDef, poly);

    Chuck_Object* b2_shape = chugin_createCkObj("b2_Shape", false, SHRED);
    *(b2ShapeId*)OBJ_MEMBER_DATA(b2_shape, b2_shape_data_offset) = shape_id;

    RETURN->v_object = b2_shape;
}

void ulib_box2d_query(Chuck_DL_Query* QUERY)
{
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

    BEGIN_CLASS("b2_ShapeDef", "Object");
    END_CLASS();

    BEGIN_CLASS("b2_WorldDef", "Object");
    b2_world_def_data_offset = MVAR("int", "@b2_world_def_data", false);
    CTOR(b2_WorldDef_ctor);
    DTOR(b2_WorldDef_dtor);
    END_CLASS();

    BEGIN_CLASS("b2_BodyDef", "Object");
    CTOR(b2_body_def_ctor);
    DTOR(b2_body_def_dtor);

    MFUN(b2_body_get_type, "int", "type");
    DOC_FUNC("The body type: static, kinematic, or dynamic");

    MFUN(b2_body_set_type, "void", "type");
    ARG("int", "b2_BodyType");
    DOC_FUNC("The body type: static, kinematic, or dynamic");

    MFUN(b2_body_get_position, "vec2", "position");
    DOC_FUNC("Get the world position of the body's origin.");

    MFUN(b2_body_set_position, "void", "position");
    ARG("vec2", "position");
    DOC_FUNC(
      "The initial world position of the body. Bodies should be created with "
      "the desired position."
      "@note Creating bodies at the origin and then moving them nearly doubles "
      "the cost of body creation, especially"
      "if the body is moved after shapes have been added.");

    MFUN(b2_body_get_angle, "float", "angle");
    DOC_FUNC("Get the world angle of the body in radians.");

    MFUN(b2_body_set_angle, "void", "angle");
    ARG("float", "angle");
    DOC_FUNC("The initial world angle of the body in radians.");

    MFUN(b2_body_get_linear_velocity, "vec2", "linearVelocity");

    MFUN(b2_body_set_linear_velocity, "void", "linearVelocity");
    ARG("vec2", "linearVelocity");
    DOC_FUNC(
      "The initial linear velocity of the body's origin. Typically in meters "
      "per second.");

    MFUN(b2_body_get_angular_velocity, "float", "angularVelocity");

    MFUN(b2_body_set_angular_velocity, "void", "angularVelocity");
    ARG("float", "angularVelocity");
    DOC_FUNC("The initial angular velocity of the body in radians per second.");

    MFUN(b2_body_get_linear_damping, "float", "linearDamping");

    MFUN(b2_body_set_linear_damping, "void", "linearDamping");
    ARG("float", "linearDamping");
    DOC_FUNC(
      "Linear damping is use to reduce the linear velocity. The damping "
      "parameter"
      "can be larger than 1 but the damping effect becomes sensitive to the"
      "time step when the damping parameter is large."
      "Generally linear damping is undesirable because it makes objects move "
      "slowly"
      "as if they are floating.");

    MFUN(b2_body_get_angular_damping, "float", "angularDamping");

    MFUN(b2_body_set_angular_damping, "void", "angularDamping");
    ARG("float", "angularDamping");
    DOC_FUNC(
      "Angular damping is use to reduce the angular velocity. The damping "
      "parameter"
      "can be larger than 1.0f but the damping effect becomes sensitive to the"
      "time step when the damping parameter is large."
      "Angular damping can be use slow down rotating bodies.");

    MFUN(b2_body_get_gravity_scale, "float", "gravityScale");

    MFUN(b2_body_set_gravity_scale, "void", "gravityScale");
    ARG("float", "gravityScale");
    DOC_FUNC("Scale the gravity applied to this body.");

    MFUN(b2_body_get_sleep_threshold, "float", "sleepThreshold");

    MFUN(b2_body_set_sleep_threshold, "void", "sleepThreshold");
    ARG("float", "sleepThreshold");
    DOC_FUNC("Sleep velocity threshold, default is 0.05 meter per second");

    MFUN(b2_body_get_enable_sleep, "int", "enableSleep");

    MFUN(b2_body_set_enable_sleep, "void", "enableSleep");
    ARG("int", "enableSleep");
    DOC_FUNC("Set this flag to false if this body should never fall asleep.");

    MFUN(b2_body_get_awake, "int", "isAwake");

    MFUN(b2_body_set_awake, "void", "isAwake");
    ARG("int", "awake");
    DOC_FUNC("Is this body initially awake or sleeping?");

    MFUN(b2_body_get_fixed_rotation, "int", "fixedRotation");

    MFUN(b2_body_set_fixed_rotation, "void", "fixedRotation");
    ARG("int", "fixedRotation");
    DOC_FUNC(
      "Should this body be prevented from rotating? Useful for characters.");

    MFUN(b2_body_get_bullet, "int", "isBullet");

    MFUN(b2_body_set_bullet, "void", "isBullet");
    ARG("int", "bullet");
    DOC_FUNC(
      "Treat this body as high speed object that performs continuous collision "
      "detection"
      "against dynamic and kinematic bodies, but not other bullet bodies."
      "@warning Bullets should be used sparingly. They are not a solution for "
      "general dynamic-versus-dynamic"
      "continuous collision. They may interfere with joint constraints.");

    MFUN(b2_body_get_enabled, "int", "isEnabled");

    MFUN(b2_body_set_enabled, "void", "isEnabled");
    ARG("int", "enabled");
    DOC_FUNC(
      "Used to disable a body. A disabled body does not move or collide.");

    MFUN(b2_body_get_automatic_mass, "int", "automaticMass");

    MFUN(b2_body_set_automatic_mass, "void", "automaticMass");
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

    BEGIN_CLASS("b2_Body", "Object");
    DOC_CLASS("Don't create bodies directly. Use b2_World.createBody instead.");
    b2_body_data_offset = MVAR("int", "@b2_body_data", false);

    MFUN(b2_body_create_polygon_shape, "b2_Shape", "createPolygonShape");
    ARG("b2_ShapeDef", "shapeDef");
    ARG("b2_Polygon", "polygon");
    DOC_FUNC(
      "Create a polygon shape and attach it to a body. The shape definition and"
      "geometry are fully cloned. Contacts are not created until the next time"
      "step."
      "@return the shape id for accessing the shape");

    END_CLASS();

    BEGIN_CLASS("b2_World", "Object");
    b2_world_data_offset = MVAR("int", "@b2_world_data", false);
    CTOR(b2_world_ctor);

    MFUN(b2_world_create_body, "b2_Body", "createBody");
    ARG("b2_BodyDef", "bodyDef");

    END_CLASS();
}
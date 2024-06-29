#include <box2d/box2d.h>
#include <chuck/chugin.h>

#include "ulib_helper.h"

/*
Experiment to integrate Box2D without OOP.
This is true to the original Box2D API, and has better performance.
Box2D worlds/bodies/shapes are identified by integer ids stored as t_CKINT.
These integers are used directly, rather than wrapping in a class, like
b2_World. So we avoid a cache miss from dereferencing the chuck object pointer.
This also simplifies the implementation; don't need to worry about
constructors/destructors/refcounting.

Downsides:
- slightly more verbose. world.createBody(body_def) --> B2.createBody(world_id,
body_def)
- no type safety. Everything is an int, rather than b2_World, b2_Body etc.
    - this will be resolved if chuck adds typedef
*/

// make sure we can fit b2 ids within a t_CKINT
static_assert(sizeof(void*) == sizeof(t_CKUINT), "pointer size mismatch");
static_assert(sizeof(b2WorldId) <= sizeof(t_CKINT), "b2Worldsize mismatch");
static_assert(sizeof(b2BodyId) <= sizeof(t_CKINT), "b2Body size mismatch");
static_assert(sizeof(b2ShapeId) <= sizeof(t_CKINT), "b2Shape size mismatch");

// accessors
#define GET_B2_ID(type, ptr) (*(type*)ptr)
#define RETURN_B2_ID(type, id) *((type*)&(RETURN->v_int)) = (id)

#define OBJ_MEMBER_B2_ID(type, ckobj, offset)                                  \
    (*(type*)OBJ_MEMBER_DATA(ckobj, offset))

#define OBJ_MEMBER_B2_PTR(type, ckobj, offset)                                 \
    (*(type**)OBJ_MEMBER_DATA(ckobj, offset))

b2BodyType ckint_to_b2BodyType(t_CKINT type)
{
    switch (type) {
        case 0: return b2_staticBody;
        case 1: return b2_kinematicBody;
        case 2: return b2_dynamicBody;
        default: return b2_staticBody;
    }
}

// ckobj data offsets --------------------------------------------
// b2_WorldDef
static t_CKUINT b2_WorldDef_gravity_offset                = 0;
static t_CKUINT b2_WorldDef_restitutionThreshold_offset   = 0;
static t_CKUINT b2_WorldDef_contactPushoutVelocity_offset = 0;
static t_CKUINT b2_WorldDef_hitEventThreshold_offset      = 0;
static t_CKUINT b2_WorldDef_contactHertz_offset           = 0;
static t_CKUINT b2_WorldDef_contactDampingRatio_offset    = 0;
static t_CKUINT b2_WorldDef_jointHertz_offset             = 0;
static t_CKUINT b2_WorldDef_jointDampingRatio_offset      = 0;
static t_CKUINT b2_WorldDef_enableSleep_offset            = 0;
static t_CKUINT b2_WorldDef_enableContinous_offset        = 0;
static t_CKUINT b2_WorldDef_workerCount_offset            = 0;
CK_DLL_CTOR(b2_WorldDef_ctor);

static void ckobj_to_b2WorldDef(CK_DL_API API, b2WorldDef* obj,
                                Chuck_Object* ckobj);
// static void b2WorldDef_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
//                                 b2WorldDef* obj);

// b2_BodyMoveEvent
static t_CKUINT b2_BodyMoveEvent_pos_offset        = 0;
static t_CKUINT b2_BodyMoveEvent_rot_offset        = 0;
static t_CKUINT b2_BodyMoveEvent_bodyId_offset     = 0;
static t_CKUINT b2_BodyMoveEvent_fellAsleep_offset = 0;

static Arena b2_body_move_event_pool;
static void b2BodyMoveEvent_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                                     b2BodyMoveEvent* obj);

// b2_Filter
static t_CKUINT b2_Filter_categoryBits_offset = 0;
static t_CKUINT b2_Filter_maskBits_offset     = 0;
static t_CKUINT b2_Filter_groupIndex_offset   = 0;
CK_DLL_CTOR(b2_Filter_ctor);

static void b2Filter_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                              b2Filter* obj);
static void ckobj_to_b2Filter(CK_DL_API API, b2Filter* obj,
                              Chuck_Object* ckobj);

// b2_ShapeDef
static t_CKUINT b2_ShapeDef_friction_offset             = 0;
static t_CKUINT b2_ShapeDef_restitution_offset          = 0;
static t_CKUINT b2_ShapeDef_density_offset              = 0;
static t_CKUINT b2_ShapeDef_filter_offset               = 0;
static t_CKUINT b2_ShapeDef_isSensor_offset             = 0;
static t_CKUINT b2_ShapeDef_enableSensorEvents_offset   = 0;
static t_CKUINT b2_ShapeDef_enableContactEvents_offset  = 0;
static t_CKUINT b2_ShapeDef_enableHitEvents_offset      = 0;
static t_CKUINT b2_ShapeDef_enablePreSolveEvents_offset = 0;
static t_CKUINT b2_ShapeDef_forceContactCreation_offset = 0;
CK_DLL_CTOR(b2_ShapeDef_ctor);

static void b2ShapeDef_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                                b2ShapeDef* obj);
static void ckobj_to_b2ShapeDef(CK_DL_API API, b2ShapeDef* obj,
                                Chuck_Object* ckobj);

// b2_Polygon
static t_CKUINT b2_polygon_data_offset = 0;

// b2_BodyDef
static t_CKUINT b2_BodyDef_type_offset            = 0;
static t_CKUINT b2_BodyDef_position_offset        = 0;
static t_CKUINT b2_BodyDef_angle_offset           = 0;
static t_CKUINT b2_BodyDef_linearVelocity_offset  = 0;
static t_CKUINT b2_BodyDef_angularVelocity_offset = 0;
static t_CKUINT b2_BodyDef_linearDamping_offset   = 0;
static t_CKUINT b2_BodyDef_angularDamping_offset  = 0;
static t_CKUINT b2_BodyDef_gravityScale_offset    = 0;
static t_CKUINT b2_BodyDef_sleepThreshold_offset  = 0;
static t_CKUINT b2_BodyDef_enableSleep_offset     = 0;
static t_CKUINT b2_BodyDef_isAwake_offset         = 0;
static t_CKUINT b2_BodyDef_fixedRotation_offset   = 0;
static t_CKUINT b2_BodyDef_isBullet_offset        = 0;
static t_CKUINT b2_BodyDef_isEnabled_offset       = 0;
static t_CKUINT b2_BodyDef_automaticMass_offset   = 0;
CK_DLL_CTOR(b2_BodyDef_ctor);

static void b2BodyDef_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                               b2BodyDef* obj);
static void ckobj_to_b2BodyDef(CK_DL_API API, b2BodyDef* obj,
                               Chuck_Object* ckobj);

// API ----------------------------------------------------------------

// b2
CK_DLL_SFUN(chugl_set_b2_world);
CK_DLL_SFUN(b2_CreateWorld);
CK_DLL_SFUN(b2_DestroyWorld);

CK_DLL_SFUN(b2_CreateBody);
CK_DLL_SFUN(b2_DestroyBody);

CK_DLL_SFUN(b2_CreatePolygonShape);

// b2_world
CK_DLL_SFUN(b2_World_IsValid);
CK_DLL_SFUN(b2_World_GetBodyEvents);

// b2_Polygon
CK_DLL_DTOR(b2_polygon_dtor);
CK_DLL_SFUN(b2_polygon_make_box);

// b2_Shape
// TODO other shape creation functions

// b2_Body
CK_DLL_SFUN(b2_body_is_valid);
CK_DLL_SFUN(b2_body_get_position);
CK_DLL_SFUN(b2_body_set_type);
CK_DLL_SFUN(b2_body_get_type);
CK_DLL_SFUN(b2_body_get_rotation);
CK_DLL_SFUN(b2_body_get_angle);
CK_DLL_SFUN(b2_body_set_transform);
CK_DLL_SFUN(b2_body_get_local_point);
CK_DLL_SFUN(b2_body_get_world_point);
CK_DLL_SFUN(b2_body_get_local_vector);
CK_DLL_SFUN(b2_body_get_world_vector);
CK_DLL_SFUN(b2_body_get_linear_velocity);
CK_DLL_SFUN(b2_body_set_linear_velocity);
CK_DLL_SFUN(b2_body_get_angular_velocity);
CK_DLL_SFUN(b2_body_set_angular_velocity);
CK_DLL_SFUN(b2_body_apply_force);
CK_DLL_SFUN(b2_body_apply_force_to_center);
CK_DLL_SFUN(b2_body_apply_torque);
CK_DLL_SFUN(b2_body_apply_linear_impulse);
CK_DLL_SFUN(b2_body_apply_linear_impulse_to_center);
CK_DLL_SFUN(b2_body_apply_angular_impulse);
CK_DLL_SFUN(b2_body_get_mass);
CK_DLL_SFUN(b2_body_get_inertia);
CK_DLL_SFUN(b2_body_get_local_center_of_mass);
CK_DLL_SFUN(b2_body_get_world_center_of_mass);
CK_DLL_SFUN(b2_body_apply_mass_from_shapes);
CK_DLL_SFUN(b2_body_set_linear_damping);
CK_DLL_SFUN(b2_body_get_linear_damping);
CK_DLL_SFUN(b2_body_set_angular_damping);
CK_DLL_SFUN(b2_body_get_angular_damping);
CK_DLL_SFUN(b2_body_set_gravity_scale);
CK_DLL_SFUN(b2_body_get_gravity_scale);
CK_DLL_SFUN(b2_body_is_awake);
CK_DLL_SFUN(b2_body_set_awake);
CK_DLL_SFUN(b2_body_enable_sleep);
CK_DLL_SFUN(b2_body_is_sleep_enabled);
CK_DLL_SFUN(b2_body_set_sleep_threshold);
CK_DLL_SFUN(b2_body_get_sleep_threshold);
CK_DLL_SFUN(b2_body_is_enabled);
CK_DLL_SFUN(b2_body_disable);
CK_DLL_SFUN(b2_body_enable);
CK_DLL_SFUN(b2_body_set_fixed_rotation);
CK_DLL_SFUN(b2_body_is_fixed_rotation);
CK_DLL_SFUN(b2_body_set_bullet);
CK_DLL_SFUN(b2_body_is_bullet);
CK_DLL_SFUN(b2_body_enable_hit_events);
CK_DLL_SFUN(b2_body_get_shape_count);
CK_DLL_SFUN(b2_body_get_shapes);
CK_DLL_SFUN(b2_body_get_joint_count);
CK_DLL_SFUN(b2_body_get_contact_capacity);
CK_DLL_SFUN(b2_body_compute_aabb);

void ulib_box2d_query(Chuck_DL_Query* QUERY)
{
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

    // b2_BodyDef --------------------------------------
    BEGIN_CLASS("b2_BodyDef", "Object");
    DOC_CLASS(
      "https://box2d.org/documentation_v3/group__body.html#structb2_body_def");

    CTOR(b2_BodyDef_ctor);

    b2_BodyDef_type_offset = MVAR("int", "type", false);
    DOC_VAR(
      "The body type: static, kinematic, or dynamic. Pass a b2_BodyType enum");

    b2_BodyDef_position_offset = MVAR("vec2", "position", false);
    DOC_VAR(
      "The initial world position of the body. Bodies should be created with "
      "the desired position. note Creating bodies at the origin and then "
      "moving them nearly doubles the cost of body creation, especially if the "
      "body is moved after shapes have been added.");

    b2_BodyDef_angle_offset = MVAR("float", "angle", false);
    DOC_VAR("The initial world angle of the body in radians.");

    b2_BodyDef_linearVelocity_offset = MVAR("vec2", "linearVelocity", false);
    DOC_VAR(
      "The initial linear velocity of the body's origin. Typically in meters "
      "per second.");

    b2_BodyDef_angularVelocity_offset = MVAR("float", "angularVelocity", false);
    DOC_VAR(
      "The initial angular velocity of the body. Typically in meters per "
      "second.");

    b2_BodyDef_linearDamping_offset = MVAR("float", "linearDamping", false);
    DOC_VAR(
      "Linear damping is use to reduce the linear velocity. The damping "
      "parameter can be larger than 1 but the damping effect becomes sensitive "
      "to the time step when the damping parameter is large. Generally linear "
      "damping is undesirable because it makes objects move slowly as if they "
      "are floating.");

    b2_BodyDef_angularDamping_offset = MVAR("float", "angularDamping", false);
    DOC_VAR(
      "Angular damping is use to reduce the angular velocity. The damping "
      "parameter can be larger than 1.0f but the damping effect becomes "
      "sensitive to the time step when the damping parameter is large. Angular "
      "damping can be use slow down rotating bodies.");

    b2_BodyDef_gravityScale_offset = MVAR("float", "gravityScale", false);
    DOC_VAR("Scale the gravity applied to this body. Non-dimensional.");

    b2_BodyDef_sleepThreshold_offset = MVAR("float", "sleepThreshold", false);
    DOC_VAR("Sleep velocity threshold, default is 0.05 meter per second");

    b2_BodyDef_enableSleep_offset = MVAR("int", "enableSleep", false);
    DOC_VAR("Set this flag to false if this body should never fall asleep.");

    b2_BodyDef_isAwake_offset = MVAR("int", "isAwake", false);
    DOC_VAR("Is this body initially awake or sleeping?");

    b2_BodyDef_fixedRotation_offset = MVAR("int", "fixedRotation", false);
    DOC_VAR(
      "Should this body be prevented from rotating? Useful for characters.");

    b2_BodyDef_isBullet_offset = MVAR("int", "isBullet", false);
    DOC_VAR(
      "Treat this body as high speed object that performs continuous collision "
      "detection against dynamic and kinematic bodies, but not other bullet "
      "bodies. Warning Bullets should be used sparingly. They are not a "
      "solution for general dynamic-versus-dynamic continuous collision. They "
      "may interfere with joint constraints.");

    b2_BodyDef_isEnabled_offset = MVAR("int", "isEnabled", false);
    DOC_VAR(
      "Used to disable a body. A disabled body does not move or collide.");

    b2_BodyDef_automaticMass_offset = MVAR("int", "automaticMass", false);
    DOC_VAR(
      "Automatically compute mass and related properties on this body from "
      "shapes. Triggers whenever a shape is add/removed/changed. Default is "
      "true.");

    END_CLASS(); // b2_BodyDef

    // b2_Polygon --------------------------------------
    // TODO eventually expose internals
    BEGIN_CLASS("b2_Polygon", "Object");
    DOC_CLASS(
      "Don't instantiate directly. Use helpers b2_Polygon.make* instead. "
      "https://box2d.org/documentation_v3/group__geometry.html#structb2_polygon"
      "A solid convex polygon. It is assumed that the interior of the polygon"
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

    // TODO other helpers

    END_CLASS(); // b2_Polygon

    // b2_Filter --------------------------------------
    BEGIN_CLASS("b2_Filter", "Object");
    DOC_CLASS(
      "https://box2d.org/documentation_v3/group__shape.html#structb2_filter");

    CTOR(b2_Filter_ctor);

    b2_Filter_categoryBits_offset = MVAR("int", "categoryBits", false);
    DOC_VAR("");

    b2_Filter_maskBits_offset = MVAR("int", "maskBits", false);
    DOC_VAR("");

    b2_Filter_groupIndex_offset = MVAR("int", "groupIndex", false);
    DOC_VAR("");

    END_CLASS(); // b2_Filter

    // b2_ShapeDef --------------------------------------
    BEGIN_CLASS("b2_ShapeDef", "Object");
    DOC_CLASS(
      "https://box2d.org/documentation_v3/"
      "group__shape.html#structb2_shape_def");

    CTOR(b2_ShapeDef_ctor);

    b2_ShapeDef_friction_offset = MVAR("float", "friction", false);
    DOC_VAR(
      "The Coulomb (dry) friction coefficient, usually in the range [0,1]");

    b2_ShapeDef_restitution_offset = MVAR("float", "restitution", false);
    DOC_VAR("The restitution (bounce) usually in the range [0,1]");

    b2_ShapeDef_density_offset = MVAR("float", "density", false);
    DOC_VAR("The density, usually in kg/m^2");

    b2_ShapeDef_filter_offset = MVAR("b2_Filter", "filter", false);
    DOC_VAR("Collision filtering data");

    b2_ShapeDef_isSensor_offset = MVAR("int", "isSensor", false);
    DOC_VAR(
      "A sensor shape generates overlap events but never generates a collision "
      "response");

    b2_ShapeDef_enableSensorEvents_offset
      = MVAR("int", "enableSensorEvents", false);
    DOC_VAR(
      "Enable sensor events for this shape. Only applies to kinematic and "
      "dynamic bodies. Ignored for sensors");

    b2_ShapeDef_enableContactEvents_offset
      = MVAR("int", "enableContactEvents", false);
    DOC_VAR(
      "Enable contact events for this shape. Only applies to kinematic and "
      "dynamic bodies. Ignored for sensors");

    b2_ShapeDef_enableHitEvents_offset = MVAR("int", "enableHitEvents", false);
    DOC_VAR(
      "Enable hit events for this shape. Only applies to kinematic and dynamic "
      "bodies. Ignored for sensors");

    b2_ShapeDef_enablePreSolveEvents_offset
      = MVAR("int", "enablePreSolveEvents", false);
    DOC_VAR(
      "Enable pre-solve contact events for this shape. Only applies to dynamic "
      "bodies. These are expensive and must be carefully handled due to "
      "threading. Ignored for sensors");

    b2_ShapeDef_forceContactCreation_offset
      = MVAR("int", "forceContactCreation", false);
    DOC_VAR(
      "Normally shapes on static bodies don't invoke contact creation when "
      "they are added to the world. This overrides that behavior and causes "
      "contact creation. This significantly slows down static body creation "
      "which can be important when there are many static shapes");

    END_CLASS(); // b2_ShapeDef

    // b2_WorldDef --------------------------------------
    BEGIN_CLASS("b2_WorldDef", "Object");
    DOC_CLASS("World definition used to create a simulation world.");

    CTOR(b2_WorldDef_ctor);

    b2_WorldDef_gravity_offset = MVAR("vec2", "gravity", false);
    DOC_VAR("Gravity vector. Box2D has no up-vector defined.");

    b2_WorldDef_restitutionThreshold_offset
      = MVAR("float", "restitutionThreshold", false);
    DOC_VAR(
      "Restitution velocity threshold, usually in m/s. Collisions above this "
      "speed have restitution applied (will bounce).");

    b2_WorldDef_contactPushoutVelocity_offset
      = MVAR("float", "contactPushoutVelocity", false);
    DOC_VAR(
      "This parameter controls how fast overlap is resolved and has units of "
      "meters per second");

    b2_WorldDef_hitEventThreshold_offset
      = MVAR("float", "hitEventThreshold", false);
    DOC_VAR("Threshold velocity for hit events. Usually meters per second.");

    b2_WorldDef_contactHertz_offset = MVAR("float", "contactHertz", false);
    DOC_VAR("Contact stiffness. Cycles per second.");

    b2_WorldDef_contactDampingRatio_offset
      = MVAR("float", "contactDampingRatio", false);
    DOC_VAR("Contact bounciness. Non-dimensional.");

    b2_WorldDef_jointHertz_offset = MVAR("float", "jointHertz", false);
    DOC_VAR("Joint stiffness. Cycles per second.");

    b2_WorldDef_jointDampingRatio_offset
      = MVAR("float", "jointDampingRatio", false);
    DOC_VAR("Joint bounciness. Non-dimensional.");

    b2_WorldDef_enableSleep_offset = MVAR("int", "enableSleep", false);
    DOC_VAR("Can bodies go to sleep to improve performance");

    b2_WorldDef_enableContinous_offset = MVAR("int", "enableContinuous", false);
    DOC_VAR("Enable continuous collision");

    b2_WorldDef_workerCount_offset = MVAR("int", "workerCount", false);
    DOC_VAR(
      "Number of workers to use with the provided task system. Box2D performs "
      "best when using only performance cores and accessing a single L2 cache. "
      "Efficiency cores and hyper-threading provide little benefit and may "
      "even harm performance.");

    END_CLASS(); // b2_WorldDef

    // b2BodyMoveEvent --------------------------------------
    Arena::init(&b2_body_move_event_pool, sizeof(Chuck_Object*) * 1024);

    BEGIN_CLASS("b2_BodyMoveEvent", "Object");
    DOC_CLASS(
      "Body move events triggered when a body moves. Triggered when a body "
      "moves due to simulation. Not reported for bodies moved by the user. "
      "This also has a flag to indicate that the body went to sleep so the "
      "application can also sleep that actor/entity/object associated with the "
      "body. On the other hand if the flag does not indicate the body went to "
      "sleep then the application can treat the actor/entity/object associated "
      "with the body as awake. This is an efficient way for an application to "
      "update game object transforms rather than calling functions such as "
      "b2Body_GetTransform() because this data is delivered as a contiguous "
      "array and it is only populated with bodies that have moved. @note If "
      "sleeping is disabled all dynamic and kinematic bodies will trigger move "
      "events.");

    b2_BodyMoveEvent_pos_offset = MVAR("vec2", "pos", false);
    DOC_VAR("2d position");

    b2_BodyMoveEvent_rot_offset = MVAR("float", "rot", false);
    DOC_VAR("rotation in radians around the z-axis");

    b2_BodyMoveEvent_bodyId_offset = MVAR("int", "bodyId", false);
    DOC_VAR("b2_BodyId");

    b2_BodyMoveEvent_fellAsleep_offset = MVAR("int", "fellAsleep", false);

    END_CLASS(); // b2_BodyMoveEvent

    // b2 --------------------------------------
    BEGIN_CLASS("b2", "Object");
    DOC_CLASS("documentation: https://box2d.org/");

    SFUN(chugl_set_b2_world, "void", "world");
    ARG("int", "world");
    DOC_FUNC("Set the active physics world for simulation");

    SFUN(b2_CreateWorld, "int", "createWorld");
    ARG("b2_WorldDef", "def");
    DOC_FUNC(
      "Create a world for rigid body simulation. A world contains bodies, "
      "shapes, and constraints. You make create up to 128 worlds. Each world "
      "is completely independent and may be simulated in parallel.");

    SFUN(b2_DestroyWorld, "void", "destroyWorld");
    ARG("int", "world_id");
    DOC_FUNC(
      "Destroy a world and all its contents. This will free all memory "
      "associated with the world, including bodies, shapes, and joints.");

    SFUN(b2_CreateBody, "int", "createBody");
    ARG("int", "world_id");
    ARG("b2_BodyDef", "def");
    DOC_FUNC(
      "Create a rigid body given a definition. No reference to the definition "
      "is retained. So you can create the definition on the stack and pass it "
      "as a pointer.");

    SFUN(b2_DestroyBody, "void", "destroyBody");
    ARG("int", "body_id");
    DOC_FUNC(
      "Destroy a rigid body given an id. This destroys all shapes and joints "
      "attached to the body. Do not keep references to the associated shapes "
      "and joints.");

    SFUN(b2_CreatePolygonShape, "int", "createPolygonShape");
    DOC_FUNC(
      "Create a polygon shape and attach it to a body. The shape definition "
      "and geometry are fully cloned. Contacts are not created until the next "
      "time step.  @return the shape id for accessing the shape");
    ARG("int", "body_id");
    ARG("b2_ShapeDef", "def");
    ARG("b2_Polygon", "polygon");

    // TODO control substep
    /// @param subStepCount The number of sub-steps, increasing the sub-step
    /// count can increase accuracy. Typically 4.
    // B2_API void b2World_Step(b2WorldId worldId, float timeStep, int
    // subStepCount);

    END_CLASS(); // b2

    // b2_Body ---------------------------------------
    BEGIN_CLASS("b2_Body", "Object");
    DOC_CLASS(
      " Don't create bodies directly. Use b2_World.createBody instead. "
      "https://box2d.org/documentation_v3/group__body.html");

    SFUN(b2_body_is_valid, "int", "isValid");
    ARG("int", "b2_body_id");
    DOC_FUNC("Check if a body id is valid");

    SFUN(b2_body_get_type, "int", "type");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the body type: static, kinematic, or dynamic");

    SFUN(b2_body_set_type, "void", "type");
    ARG("int", "b2_body_id");
    ARG("int", "b2_BodyType");
    DOC_FUNC(
      " Change the body type. This is an expensive operation. This "
      "automatically updates the mass properties regardless of the automatic "
      "mass setting.");

    SFUN(b2_body_get_position, "vec2", "position");
    ARG("int", "b2_body_id");
    DOC_FUNC(
      "Get the world position of a body. This is the location of the body "
      "origin.");

    SFUN(b2_body_get_rotation, "complex", "rotation");
    ARG("int", "b2_body_id");
    DOC_FUNC(
      "Get the world rotation of a body as a cosine/sine pair (complex "
      "number)");

    SFUN(b2_body_get_angle, "float", "angle");
    ARG("int", "b2_body_id");
    DOC_FUNC(" Get the body angle in radians in the range [-pi, pi]");

    SFUN(b2_body_set_transform, "void", "transform");
    ARG("int", "b2_body_id");
    ARG("vec2", "position");
    ARG("float", "angle");
    DOC_FUNC(
      "Set the world transform of a body. This acts as a teleport and is "
      "fairly expensive."
      "@note Generally you should create a body with then intended "
      "transform."
      "@see b2_BodyDef.position and b2_BodyDef.angle");

    SFUN(b2_body_get_local_point, "vec2", "localPoint");
    ARG("int", "b2_body_id");
    ARG("vec2", "worldPoint");
    DOC_FUNC("Get a local point on a body given a world point");

    SFUN(b2_body_get_world_point, "vec2", "worldPoint");
    ARG("int", "b2_body_id");
    ARG("vec2", "localPoint");
    DOC_FUNC("Get a world point on a body given a local point");

    SFUN(b2_body_get_local_vector, "vec2", "localVector");
    ARG("int", "b2_body_id");
    ARG("vec2", "worldVector");
    DOC_FUNC("Get a local vector on a body given a world vector");

    SFUN(b2_body_get_world_vector, "vec2", "worldVector");
    ARG("int", "b2_body_id");
    ARG("vec2", "localVector");
    DOC_FUNC("Get a world vector on a body given a local vector");

    SFUN(b2_body_get_linear_velocity, "vec2", "linearVelocity");
    ARG("int", "b2_body_id");
    DOC_FUNC(
      "Get the linear velocity of a body's center of mass. Typically in "
      "meters per second.");

    SFUN(b2_body_set_linear_velocity, "void", "linearVelocity");
    ARG("int", "b2_body_id");
    ARG("vec2", "linearVelocity");
    DOC_FUNC(
      "Set the linear velocity of a body. Typically in meters per second.");

    SFUN(b2_body_get_angular_velocity, "float", "angularVelocity");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the angular velocity of a body in radians per second");

    SFUN(b2_body_set_angular_velocity, "void", "angularVelocity");
    ARG("int", "b2_body_id");
    ARG("float", "angularVelocity");
    DOC_FUNC("Set the angular velocity of a body in radians per second");

    SFUN(b2_body_apply_force, "void", "force");
    ARG("int", "b2_body_id");
    ARG("vec2", "force");
    ARG("vec2", "point");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply a force at a world point. If the force is not applied at the "
      "center of mass, it will generate a torque and affect the angular "
      "velocity. This optionally wakes up the body."
      "The force is ignored if the body is not awake.");

    SFUN(b2_body_apply_force_to_center, "void", "force");
    ARG("int", "b2_body_id");
    ARG("vec2", "force");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply a force to the center of mass. This optionally wakes up the "
      "body. The force is ignored if the body is not awake.");

    SFUN(b2_body_apply_torque, "void", "torque");
    ARG("int", "b2_body_id");
    ARG("float", "torque");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply a torque. This affects the angular velocity without affecting "
      "the linear velocity."
      "This optionally wakes the body. The torque is ignored if the body "
      "is not awake.");

    SFUN(b2_body_apply_linear_impulse, "void", "impulse");
    ARG("int", "b2_body_id");
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

    SFUN(b2_body_apply_linear_impulse_to_center, "void", "impulse");
    ARG("int", "b2_body_id");
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

    SFUN(b2_body_apply_angular_impulse, "void", "impulse");
    ARG("int", "b2_body_id");
    ARG("float", "impulse");
    ARG("int", "wake");
    DOC_FUNC(
      "Apply an angular impulse. The impulse is ignored if the body is not "
      "awake. This optionally wakes the body."
      "@warning This should be used for one-shot impulses. If you need a "
      "steady"
      "force, use a force instead, which will work better with the "
      "sub-stepping solver.");

    SFUN(b2_body_get_mass, "float", "mass");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the mass of the body, typically in kilograms");

    SFUN(b2_body_get_inertia, "float", "inertia");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the inertia tensor of the body, typically in kg*m^2");

    SFUN(b2_body_get_local_center_of_mass, "vec2", "localCenterOfMass");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the center of mass position of the body in local space");

    SFUN(b2_body_get_world_center_of_mass, "vec2", "worldCenterOfMass");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the center of mass position of the body in world space");

    // SFUN(b2_body_get_mass_data, "b2_MassData", "massData");
    // ARG("int", "b2_body_id");
    // DOC_FUNC("Get the mass data for a body");

    // SFUN(b2_body_set_mass_data, "void", "massData");
    // ARG("int", "b2_body_id");
    // ARG("b2_MassData", "massData");
    // DOC_FUNC(
    //   "Override the body's mass properties. Normally this is computed "
    //   "automatically using the shape geometry and density."
    //   "This information is lost if a shape is added or removed or if the "
    //   "body type changes.");

    SFUN(b2_body_apply_mass_from_shapes, "void", "applyMassFromShapes");
    ARG("int", "b2_body_id");
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
    // SFUN(b2_body_set_automatic_mass, "void", "automaticMass");
    // ARG("int", "automaticMass");
    // DOC_FUNC(
    //   "Set the automatic mass setting. Normally this is set in b2BodyDef "
    //   "before creation.");

    // SFUN(b2_body_get_automatic_mass, "int", "automaticMass");
    // DOC_FUNC("Get the automatic mass setting");

    SFUN(b2_body_set_linear_damping, "void", "linearDamping");
    ARG("int", "b2_body_id");
    ARG("float", "linearDamping");
    DOC_FUNC(
      "Adjust the linear damping. Normally this is set in b2BodyDef before "
      "creation.");

    SFUN(b2_body_get_linear_damping, "float", "linearDamping");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the current linear damping.");

    SFUN(b2_body_set_angular_damping, "void", "angularDamping");
    ARG("int", "b2_body_id");
    ARG("float", "angularDamping");
    DOC_FUNC(
      "Adjust the angular damping. Normally this is set in b2BodyDef before "
      "creation.");

    SFUN(b2_body_get_angular_damping, "float", "angularDamping");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the current angular damping.");

    SFUN(b2_body_set_gravity_scale, "void", "gravityScale");
    ARG("int", "b2_body_id");
    ARG("float", "gravityScale");
    DOC_FUNC(
      "Adjust the gravity scale. Normally this is set in b2BodyDef before "
      "creation."
      "@see b2BodyDef::gravityScale");

    SFUN(b2_body_get_gravity_scale, "float", "gravityScale");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the current gravity scale");

    SFUN(b2_body_is_awake, "int", "isAwake");
    ARG("int", "b2_body_id");
    DOC_FUNC("Returns true if this body is awake");

    SFUN(b2_body_set_awake, "void", "awake");
    ARG("int", "b2_body_id");
    ARG("int", "awake");
    DOC_FUNC(
      "Wake a body from sleep. This wakes the entire island the body is "
      "touching."
      "@warning Putting a body to sleep will put the entire island of "
      "bodies"
      "touching this body to sleep, which can be expensive and possibly "
      "unintuitive.");

    SFUN(b2_body_enable_sleep, "void", "enableSleep");
    ARG("int", "b2_body_id");
    ARG("int", "enableSleep");
    DOC_FUNC(
      "Enable or disable sleeping for this body. If sleeping is disabled "
      "the body will wake.");

    SFUN(b2_body_is_sleep_enabled, "int", "isSleepEnabled");
    ARG("int", "b2_body_id");
    DOC_FUNC("Returns true if sleeping is enabled for this body");

    SFUN(b2_body_set_sleep_threshold, "void", "sleepThreshold");
    ARG("int", "b2_body_id");
    ARG("float", "sleepThreshold");
    DOC_FUNC("Set the sleep threshold, typically in meters per second");

    SFUN(b2_body_get_sleep_threshold, "float", "sleepThreshold");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the sleep threshold, typically in meters per second.");

    SFUN(b2_body_is_enabled, "int", "isEnabled");
    ARG("int", "b2_body_id");
    DOC_FUNC("Returns true if this body is enabled");

    SFUN(b2_body_disable, "void", "disable");
    ARG("int", "b2_body_id");
    DOC_FUNC(
      "Disable a body by removing it completely from the simulation. This "
      "is expensive.");

    SFUN(b2_body_enable, "void", "enable");
    ARG("int", "b2_body_id");
    DOC_FUNC(
      "Enable a body by adding it to the simulation. This is expensive.");

    SFUN(b2_body_set_fixed_rotation, "void", "fixedRotation");
    ARG("int", "b2_body_id");
    ARG("int", "flag");
    DOC_FUNC(
      "Set this body to have fixed rotation. This causes the mass to be "
      "reset in all cases.");

    SFUN(b2_body_is_fixed_rotation, "int", "isFixedRotation");
    ARG("int", "b2_body_id");
    DOC_FUNC("Does this body have fixed rotation?");

    SFUN(b2_body_set_bullet, "void", "bullet");
    ARG("int", "b2_body_id");
    ARG("int", "flag");
    DOC_FUNC(
      "Set this body to be a bullet. A bullet does continuous collision "
      "detection against dynamic bodies (but not other bullets).");

    SFUN(b2_body_is_bullet, "int", "isBullet");
    ARG("int", "b2_body_id");
    DOC_FUNC("Is this body a bullet?");

    SFUN(b2_body_enable_hit_events, "void", "enableHitEvents");
    ARG("int", "b2_body_id");
    ARG("int", "enableHitEvents");
    DOC_FUNC(
      "Enable/disable hit events on all shapes"
      "@see b2ShapeDef::enableHitEvents");

    SFUN(b2_body_get_shape_count, "int", "shapeCount");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the number of shapes on this body");

    // SFUN(b2_body_get_shapes, "int[]", "shapes");
    // ARG("int", "b2_body_id");
    // DOC_FUNC("Get the shape ids for all shapes on this body");

    SFUN(b2_body_get_joint_count, "int", "jointCount");
    ARG("int", "b2_body_id");
    DOC_FUNC("Get the number of joints on this body");

    // TODO add after impl joints
    // SFUN(b2_body_get_joints, "b2_Joint[]", "joints");
    // DOC_FUNC("Get the joint ids for all joints on this body");

    SFUN(b2_body_get_contact_capacity, "int", "contactCapacity");
    ARG("int", "b2_body_id");
    DOC_FUNC(
      "Get the maximum capacity required for retrieving all the touching "
      "contacts on a body");

    // TODO add after impl contacts
    // SFUN(b2_body_get_contact_data, "b2_ContactData[]", "contactData");
    // ARG("int", "b2_body_id");
    // DOC_FUNC("Get the touching contact data for a body");

    SFUN(b2_body_compute_aabb, "vec4", "computeAABB");
    // ARG("int", "b2_body_id");
    DOC_FUNC(
      "Get the current world AABB that contains all the attached shapes. Note "
      "that this may not encompass the body origin."
      "If there are no shapes attached then the returned AABB is empty and "
      "centered on the body origin."
      "The aabb is in the form of (lowerBound.x, lowerBound.y, upperBound.x, "
      "upperBound.y");

    END_CLASS(); // b2_Body

    BEGIN_CLASS("b2_Shape", "Object");
    END_CLASS(); // b2_Shape

    // b2_World --------------------------------------

    BEGIN_CLASS("b2_World", "Object");

    SFUN(b2_World_IsValid, "int", "isValid");
    ARG("int", "world_id");
    DOC_FUNC(
      "World id validation. Provides validation for up to 64K allocations.");

    // TODO debug draw
    // B2_API void b2World_Draw(b2WorldId worldId, b2DebugDraw* draw);

    SFUN(b2_World_GetBodyEvents, "void", "bodyEvents");
    ARG("int", "world_id");
    ARG("b2_BodyMoveEvent[]", "body_events");
    DOC_FUNC("Get the body events for the current time step.");

    END_CLASS(); // b2_World
}

// ============================================================================
// b2BodyMoveEvent
// ============================================================================

static void b2BodyMoveEvent_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                                     b2BodyMoveEvent* obj)
{
    OBJ_MEMBER_VEC2(ckobj, b2_BodyMoveEvent_pos_offset)
      = { obj->transform.p.x, obj->transform.p.y };
    // convert complex rot to radians
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyMoveEvent_rot_offset)
      = atan2(obj->transform.q.s, obj->transform.q.c);
    OBJ_MEMBER_B2_ID(b2BodyId, ckobj, b2_BodyMoveEvent_bodyId_offset)
      = obj->bodyId;
    OBJ_MEMBER_INT(ckobj, b2_BodyMoveEvent_fellAsleep_offset) = obj->fellAsleep;
}

// ============================================================================
// b2
// ============================================================================

CK_DLL_SFUN(chugl_set_b2_world)
{
    b2WorldId world_id = GET_B2_ID(b2WorldId, ARGS);
    CQ_PushCommand_b2World_Set(*(u32*)&world_id);
}

CK_DLL_SFUN(b2_CreateWorld)
{
    b2WorldDef def = b2DefaultWorldDef();
    // TODO impl enqueueTask and finishTask callbacks
    ckobj_to_b2WorldDef(API, &def, GET_NEXT_OBJECT(ARGS));
    RETURN_B2_ID(b2WorldId, b2CreateWorld(&def));
}

CK_DLL_SFUN(b2_DestroyWorld)
{
    b2DestroyWorld(GET_B2_ID(b2WorldId, ARGS));
}

CK_DLL_SFUN(b2_CreateBody)
{
    b2WorldId world_id = GET_B2_ID(b2WorldId, ARGS);
    GET_NEXT_INT(ARGS); // advance to next arg
    b2BodyDef body_def = b2DefaultBodyDef();
    ckobj_to_b2BodyDef(API, &body_def, GET_NEXT_OBJECT(ARGS));
    RETURN_B2_ID(b2BodyId, b2CreateBody(world_id, &body_def));
}

CK_DLL_SFUN(b2_DestroyBody)
{
    b2DestroyBody(GET_B2_ID(b2BodyId, ARGS));
}

CK_DLL_SFUN(b2_CreatePolygonShape)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance to next arg

    b2ShapeDef shape_def = b2DefaultShapeDef();
    ckobj_to_b2ShapeDef(API, &shape_def, GET_NEXT_OBJECT(ARGS));

    b2Polygon* polygon = OBJ_MEMBER_B2_PTR(b2Polygon, GET_NEXT_OBJECT(ARGS),
                                           b2_polygon_data_offset);
    // b2Polygon polygon = b2MakeBox(.5f, .5f);

    RETURN_B2_ID(b2ShapeId, b2CreatePolygonShape(body_id, &shape_def, polygon));
    // RETURN_B2_ID(b2ShapeId,
    //              b2CreatePolygonShape(body_id, &shape_def, &polygon));
}

// ============================================================================
// b2_World
// ============================================================================

CK_DLL_SFUN(b2_World_IsValid)
{
    RETURN->v_int = b2World_IsValid(GET_B2_ID(b2WorldId, ARGS));
}

CK_DLL_SFUN(b2_World_GetBodyEvents)
{
    b2WorldId world_id = GET_B2_ID(b2WorldId, ARGS);
    GET_NEXT_INT(ARGS); // advance to next arg
    Chuck_ArrayInt* body_event_array = GET_NEXT_OBJECT_ARRAY(ARGS);

    b2BodyEvents body_events = b2World_GetBodyEvents(world_id);

    // first create new body events in pool
    int pool_len = ARENA_LENGTH(&b2_body_move_event_pool, Chuck_Object*);
    for (int i = pool_len; i < body_events.moveCount; i++)
        *ARENA_PUSH_TYPE(&b2_body_move_event_pool, Chuck_Object*)
          = chugin_createCkObj("b2_BodyMoveEvent", true, SHRED);

    // TODO switch to use array_int_set after updating chuck version
    // int ck_array_len         = API->object->array_int_size(body_event_array);

    // clear array
    API->object->array_int_clear(body_event_array);

    // add new body events to array
    for (int i = 0; i < body_events.moveCount; i++) {
        Chuck_Object* ckobj
          = *ARENA_GET_TYPE(&b2_body_move_event_pool, Chuck_Object*, i);
        b2BodyMoveEvent_to_ckobj(API, ckobj, &body_events.moveEvents[i]);
        API->object->array_int_push_back(body_event_array, (t_CKUINT)ckobj);
    }
}

// ============================================================================
// b2_WorldDef
// ============================================================================
static void b2WorldDef_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                                b2WorldDef* obj)
{
    OBJ_MEMBER_VEC2(ckobj, b2_WorldDef_gravity_offset)
      = { obj->gravity.x, obj->gravity.y };
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_restitutionThreshold_offset)
      = obj->restitutionThreshold;
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_contactPushoutVelocity_offset)
      = obj->contactPushoutVelocity;
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_hitEventThreshold_offset)
      = obj->hitEventThreshold;
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_contactHertz_offset)
      = obj->contactHertz;
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_contactDampingRatio_offset)
      = obj->contactDampingRatio;
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_jointHertz_offset) = obj->jointHertz;
    OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_jointDampingRatio_offset)
      = obj->jointDampingRatio;
    OBJ_MEMBER_INT(ckobj, b2_WorldDef_enableSleep_offset) = obj->enableSleep;
    OBJ_MEMBER_INT(ckobj, b2_WorldDef_enableContinous_offset)
      = obj->enableContinous;
    OBJ_MEMBER_INT(ckobj, b2_WorldDef_workerCount_offset) = obj->workerCount;
}

static void ckobj_to_b2WorldDef(CK_DL_API API, b2WorldDef* obj,
                                Chuck_Object* ckobj)
{
    t_CKVEC2 gravity_vec2 = OBJ_MEMBER_VEC2(ckobj, b2_WorldDef_gravity_offset);
    obj->gravity          = { (float)gravity_vec2.x, (float)gravity_vec2.y };
    obj->restitutionThreshold
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_restitutionThreshold_offset);
    obj->contactPushoutVelocity = (float)OBJ_MEMBER_FLOAT(
      ckobj, b2_WorldDef_contactPushoutVelocity_offset);
    obj->hitEventThreshold
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_hitEventThreshold_offset);
    obj->contactHertz
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_contactHertz_offset);
    obj->contactDampingRatio
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_contactDampingRatio_offset);
    obj->jointHertz
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_jointHertz_offset);
    obj->jointDampingRatio
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_WorldDef_jointDampingRatio_offset);
    obj->enableSleep = OBJ_MEMBER_INT(ckobj, b2_WorldDef_enableSleep_offset);
    obj->enableContinous
      = OBJ_MEMBER_INT(ckobj, b2_WorldDef_enableContinous_offset);
    obj->workerCount = OBJ_MEMBER_INT(ckobj, b2_WorldDef_workerCount_offset);
}

CK_DLL_CTOR(b2_WorldDef_ctor)
{
    b2WorldDef default_world_def = b2DefaultWorldDef();
    b2WorldDef_to_ckobj(API, SELF, &default_world_def);
}

// ============================================================================
// b2_Filter
// ============================================================================

static void b2Filter_to_ckobj(CK_DL_API API, Chuck_Object* ckobj, b2Filter* obj)
{
    OBJ_MEMBER_INT(ckobj, b2_Filter_categoryBits_offset) = obj->categoryBits;
    OBJ_MEMBER_INT(ckobj, b2_Filter_maskBits_offset)     = obj->maskBits;
    ASSERT(OBJ_MEMBER_INT(ckobj, b2_Filter_maskBits_offset) == obj->maskBits);
    OBJ_MEMBER_INT(ckobj, b2_Filter_groupIndex_offset) = obj->groupIndex;
}

static void ckobj_to_b2Filter(CK_DL_API API, b2Filter* obj, Chuck_Object* ckobj)
{
    obj->categoryBits = OBJ_MEMBER_INT(ckobj, b2_Filter_categoryBits_offset);
    obj->maskBits     = OBJ_MEMBER_INT(ckobj, b2_Filter_maskBits_offset);
    obj->groupIndex   = OBJ_MEMBER_INT(ckobj, b2_Filter_groupIndex_offset);
}

CK_DLL_CTOR(b2_Filter_ctor)
{
    b2Filter default_filter = b2DefaultFilter();
    b2Filter_to_ckobj(API, SELF, &default_filter);
}

// ============================================================================
// b2_ShapeDef
// ============================================================================

static void b2ShapeDef_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                                b2ShapeDef* obj)
{
    OBJ_MEMBER_FLOAT(ckobj, b2_ShapeDef_friction_offset)    = obj->friction;
    OBJ_MEMBER_FLOAT(ckobj, b2_ShapeDef_restitution_offset) = obj->restitution;
    OBJ_MEMBER_FLOAT(ckobj, b2_ShapeDef_density_offset)     = obj->density;
    // b2Filter_to_ckobj(API, OBJ_MEMBER_OBJECT(ckobj,
    // b2_ShapeDef_filter_offset),
    //                   &obj->filter);
    b2Filter_to_ckobj(
      API, (Chuck_Object*)OBJ_MEMBER_UINT(ckobj, b2_ShapeDef_filter_offset),
      &obj->filter);
    OBJ_MEMBER_INT(ckobj, b2_ShapeDef_isSensor_offset) = obj->isSensor;
    OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enableSensorEvents_offset)
      = obj->enableSensorEvents;
    OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enableContactEvents_offset)
      = obj->enableContactEvents;
    OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enableHitEvents_offset)
      = obj->enableHitEvents;
    OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enablePreSolveEvents_offset)
      = obj->enablePreSolveEvents;
    OBJ_MEMBER_INT(ckobj, b2_ShapeDef_forceContactCreation_offset)
      = obj->forceContactCreation;
}

static void ckobj_to_b2ShapeDef(CK_DL_API API, b2ShapeDef* obj,
                                Chuck_Object* ckobj)
{
    obj->friction = (float)OBJ_MEMBER_FLOAT(ckobj, b2_ShapeDef_friction_offset);
    obj->restitution
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_ShapeDef_restitution_offset);
    obj->density = (float)OBJ_MEMBER_FLOAT(ckobj, b2_ShapeDef_density_offset);
    ckobj_to_b2Filter(API, &obj->filter,
                      OBJ_MEMBER_OBJECT(ckobj, b2_ShapeDef_filter_offset));
    obj->isSensor = OBJ_MEMBER_INT(ckobj, b2_ShapeDef_isSensor_offset);
    obj->enableSensorEvents
      = OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enableSensorEvents_offset);
    obj->enableContactEvents
      = OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enableContactEvents_offset);
    obj->enableHitEvents
      = OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enableHitEvents_offset);
    obj->enablePreSolveEvents
      = OBJ_MEMBER_INT(ckobj, b2_ShapeDef_enablePreSolveEvents_offset);
    obj->forceContactCreation
      = OBJ_MEMBER_INT(ckobj, b2_ShapeDef_forceContactCreation_offset);
}

CK_DLL_CTOR(b2_ShapeDef_ctor)
{
    // https://github.com/ccrma/chuck/issues/449
    // member vars which are themselves Chuck_Objects are NOT instantiated
    // instantiating manually
    OBJ_MEMBER_OBJECT(SELF, b2_ShapeDef_filter_offset) = chugin_createCkObj(
      "b2_Filter", true, SHRED); // adding refcount just in case gc isn't set up

    b2ShapeDef default_shape_def = b2DefaultShapeDef();
    b2ShapeDef_to_ckobj(API, SELF, &default_shape_def);
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
    b2Polygon* box_ptr = new b2Polygon(b2MakeBox(hx, hy));

    Chuck_Object* poly = chugin_createCkObj("b2_Polygon", false, SHRED);
    OBJ_MEMBER_UINT(poly, b2_polygon_data_offset) = (t_CKUINT)box_ptr;

    RETURN->v_object = poly;
}

// ============================================================================
// b2_BodyDef
// ============================================================================

static void b2BodyDef_to_ckobj(CK_DL_API API, Chuck_Object* ckobj,
                               b2BodyDef* obj)
{
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_type_offset) = obj->type;
    OBJ_MEMBER_VEC2(ckobj, b2_BodyDef_position_offset)
      = { obj->position.x, obj->position.y };
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_angle_offset) = obj->angle;
    OBJ_MEMBER_VEC2(ckobj, b2_BodyDef_linearVelocity_offset)
      = { obj->linearVelocity.x, obj->linearVelocity.y };
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_angularVelocity_offset)
      = obj->angularVelocity;
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_linearDamping_offset)
      = obj->linearDamping;
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_angularDamping_offset)
      = obj->angularDamping;
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_gravityScale_offset) = obj->gravityScale;
    OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_sleepThreshold_offset)
      = obj->sleepThreshold;
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_enableSleep_offset)   = obj->enableSleep;
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_isAwake_offset)       = obj->isAwake;
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_fixedRotation_offset) = obj->fixedRotation;
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_isBullet_offset)      = obj->isBullet;
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_isEnabled_offset)     = obj->isEnabled;
    OBJ_MEMBER_INT(ckobj, b2_BodyDef_automaticMass_offset) = obj->automaticMass;
}

static void ckobj_to_b2BodyDef(CK_DL_API API, b2BodyDef* obj,
                               Chuck_Object* ckobj)
{
    obj->type = (b2BodyType)OBJ_MEMBER_INT(ckobj, b2_BodyDef_type_offset);
    t_CKVEC2 position_vec2 = OBJ_MEMBER_VEC2(ckobj, b2_BodyDef_position_offset);
    obj->position          = { (float)position_vec2.x, (float)position_vec2.y };
    obj->angle = (float)OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_angle_offset);
    t_CKVEC2 linearVelocity_vec2
      = OBJ_MEMBER_VEC2(ckobj, b2_BodyDef_linearVelocity_offset);
    obj->linearVelocity
      = { (float)linearVelocity_vec2.x, (float)linearVelocity_vec2.y };
    obj->angularVelocity
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_angularVelocity_offset);
    obj->linearDamping
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_linearDamping_offset);
    obj->angularDamping
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_angularDamping_offset);
    obj->gravityScale
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_gravityScale_offset);
    obj->sleepThreshold
      = (float)OBJ_MEMBER_FLOAT(ckobj, b2_BodyDef_sleepThreshold_offset);
    obj->enableSleep   = OBJ_MEMBER_INT(ckobj, b2_BodyDef_enableSleep_offset);
    obj->isAwake       = OBJ_MEMBER_INT(ckobj, b2_BodyDef_isAwake_offset);
    obj->fixedRotation = OBJ_MEMBER_INT(ckobj, b2_BodyDef_fixedRotation_offset);
    obj->isBullet      = OBJ_MEMBER_INT(ckobj, b2_BodyDef_isBullet_offset);
    obj->isEnabled     = OBJ_MEMBER_INT(ckobj, b2_BodyDef_isEnabled_offset);
    obj->automaticMass = OBJ_MEMBER_INT(ckobj, b2_BodyDef_automaticMass_offset);
}

CK_DLL_CTOR(b2_BodyDef_ctor)
{
    b2BodyDef default_body_def = b2DefaultBodyDef();
    b2BodyDef_to_ckobj(API, SELF, &default_body_def);
}

// ============================================================================
// b2_Body
// ============================================================================

CK_DLL_SFUN(b2_body_is_valid)
{
    RETURN->v_int = b2Body_IsValid(GET_B2_ID(b2BodyId, ARGS));
}

CK_DLL_SFUN(b2_body_get_type)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_GetType(body_id);
}

CK_DLL_SFUN(b2_body_set_type)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetType(body_id, ckint_to_b2BodyType(GET_NEXT_INT(ARGS)));
}

CK_DLL_SFUN(b2_body_get_position)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Vec2 pos     = b2Body_GetPosition(body_id);
    RETURN->v_vec2 = { pos.x, pos.y };
}

CK_DLL_SFUN(b2_body_get_rotation)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Rot rot         = b2Body_GetRotation(body_id);
    RETURN->v_complex = { rot.c, rot.s };
}

CK_DLL_SFUN(b2_body_get_angle)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetAngle(body_id);
}

CK_DLL_SFUN(b2_body_set_transform)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 pos = GET_NEXT_VEC2(ARGS);
    float angle  = GET_NEXT_FLOAT(ARGS);
    b2Body_SetTransform(body_id, { (float)pos.x, (float)pos.y }, angle);
}

CK_DLL_SFUN(b2_body_get_local_point)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 world_point = GET_NEXT_VEC2(ARGS);
    b2Vec2 local_point   = b2Body_GetLocalPoint(
      body_id, { (float)world_point.x, (float)world_point.y });
    RETURN->v_vec2 = { local_point.x, local_point.y };
}

CK_DLL_SFUN(b2_body_get_world_point)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 local_point = GET_NEXT_VEC2(ARGS);
    b2Vec2 world_point   = b2Body_GetWorldPoint(
      body_id, { (float)local_point.x, (float)local_point.y });
    RETURN->v_vec2 = { world_point.x, world_point.y };
}

CK_DLL_SFUN(b2_body_get_local_vector)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 world_vector = GET_NEXT_VEC2(ARGS);
    b2Vec2 local_vector   = b2Body_GetLocalVector(
      body_id, { (float)world_vector.x, (float)world_vector.y });
    RETURN->v_vec2 = { local_vector.x, local_vector.y };
}

CK_DLL_SFUN(b2_body_get_world_vector)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 local_vector = GET_NEXT_VEC2(ARGS);
    b2Vec2 world_vector   = b2Body_GetWorldVector(
      body_id, { (float)local_vector.x, (float)local_vector.y });
    RETURN->v_vec2 = { world_vector.x, world_vector.y };
}

CK_DLL_SFUN(b2_body_get_linear_velocity)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Vec2 vel     = b2Body_GetLinearVelocity(body_id);
    RETURN->v_vec2 = { vel.x, vel.y };
}

CK_DLL_SFUN(b2_body_set_linear_velocity)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 vel = GET_NEXT_VEC2(ARGS);
    b2Body_SetLinearVelocity(body_id, { (float)vel.x, (float)vel.y });
}

CK_DLL_SFUN(b2_body_get_angular_velocity)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetAngularVelocity(body_id);
}

CK_DLL_SFUN(b2_body_set_angular_velocity)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetAngularVelocity(body_id, GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(b2_body_apply_force)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 force = GET_NEXT_VEC2(ARGS);
    t_CKVEC2 point = GET_NEXT_VEC2(ARGS);
    t_CKINT wake   = GET_NEXT_INT(ARGS);
    b2Body_ApplyForce(body_id, { (float)force.x, (float)force.y },
                      { (float)point.x, (float)point.y }, wake);
}

CK_DLL_SFUN(b2_body_apply_force_to_center)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 force = GET_NEXT_VEC2(ARGS);
    t_CKINT wake   = GET_NEXT_INT(ARGS);
    b2Body_ApplyForceToCenter(body_id, { (float)force.x, (float)force.y },
                              wake);
}

CK_DLL_SFUN(b2_body_apply_torque)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKFLOAT torque = GET_NEXT_FLOAT(ARGS);
    t_CKINT wake     = GET_NEXT_INT(ARGS);
    b2Body_ApplyTorque(body_id, torque, wake);
}

CK_DLL_SFUN(b2_body_apply_linear_impulse)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 impulse = GET_NEXT_VEC2(ARGS);
    t_CKVEC2 point   = GET_NEXT_VEC2(ARGS);
    t_CKINT wake     = GET_NEXT_INT(ARGS);
    b2Body_ApplyLinearImpulse(body_id, { (float)impulse.x, (float)impulse.y },
                              { (float)point.x, (float)point.y }, wake);
}

CK_DLL_SFUN(b2_body_apply_linear_impulse_to_center)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKVEC2 impulse = GET_NEXT_VEC2(ARGS);
    t_CKINT wake     = GET_NEXT_INT(ARGS);
    b2Body_ApplyLinearImpulseToCenter(
      body_id, { (float)impulse.x, (float)impulse.y }, wake);
}

CK_DLL_SFUN(b2_body_apply_angular_impulse)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    t_CKFLOAT impulse = GET_NEXT_FLOAT(ARGS);
    t_CKINT wake      = GET_NEXT_INT(ARGS);
    b2Body_ApplyAngularImpulse(body_id, impulse, wake);
}

CK_DLL_SFUN(b2_body_get_mass)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetMass(body_id);
}

CK_DLL_SFUN(b2_body_get_inertia)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetInertiaTensor(body_id);
}

CK_DLL_SFUN(b2_body_get_local_center_of_mass)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Vec2 center  = b2Body_GetLocalCenterOfMass(body_id);
    RETURN->v_vec2 = { center.x, center.y };
}

CK_DLL_SFUN(b2_body_get_world_center_of_mass)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Vec2 center  = b2Body_GetWorldCenterOfMass(body_id);
    RETURN->v_vec2 = { center.x, center.y };
}

// CK_DLL_SFUN(b2_body_get_mass_data)
// {
//     b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
//     GET_NEXT_INT(ARGS); // advance
//     b2MassData data           = b2Body_GetMassData(body_id);
//     Chuck_Object* mass_data   = chugin_createCkObj("b2_MassData", false,
//     SHRED); b2MassData* mass_data_ptr = new b2MassData;

//     OBJ_MEMBER_UINT(mass_data, b2_mass_data_offset) =
//     (t_CKUINT)mass_data_ptr;

//     // TODO: impl after Ge gets back on API->object->create() not invoking
//     // default ctor

//     memcpy(mass_data_ptr, &data, sizeof(data));

//     RETURN->v_object = mass_data;
// }

// CK_DLL_SFUN(b2_body_set_mass_data)
// {
//     b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
//     GET_NEXT_INT(ARGS); // advance
//     Chuck_Object* mass_data_obj = GET_NEXT_OBJECT(ARGS);
//     b2MassData* mass_data
//       = (b2MassData*)OBJ_MEMBER_UINT(mass_data_obj, b2_mass_data_offset);
//     b2Body_SetMassData(body_id, *mass_data);
// }

CK_DLL_SFUN(b2_body_apply_mass_from_shapes)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_ApplyMassFromShapes(body_id);
}

CK_DLL_SFUN(b2_body_set_linear_damping)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetLinearDamping(body_id, GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(b2_body_get_linear_damping)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetLinearDamping(body_id);
}

CK_DLL_SFUN(b2_body_set_angular_damping)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetAngularDamping(body_id, GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(b2_body_get_angular_damping)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetAngularDamping(body_id);
}

CK_DLL_SFUN(b2_body_set_gravity_scale)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetGravityScale(body_id, GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(b2_body_get_gravity_scale)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetGravityScale(body_id);
}

CK_DLL_SFUN(b2_body_is_awake)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_IsAwake(body_id);
}

CK_DLL_SFUN(b2_body_set_awake)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetAwake(body_id, GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(b2_body_enable_sleep)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_EnableSleep(body_id, GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(b2_body_is_sleep_enabled)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_IsSleepEnabled(body_id);
}

CK_DLL_SFUN(b2_body_set_sleep_threshold)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetSleepThreshold(body_id, GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(b2_body_get_sleep_threshold)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_float = b2Body_GetSleepThreshold(body_id);
}

CK_DLL_SFUN(b2_body_is_enabled)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_IsEnabled(body_id);
}

CK_DLL_SFUN(b2_body_disable)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_Disable(body_id);
}

CK_DLL_SFUN(b2_body_enable)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_Enable(body_id);
}

CK_DLL_SFUN(b2_body_set_fixed_rotation)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetFixedRotation(body_id, GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(b2_body_is_fixed_rotation)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_IsFixedRotation(body_id);
}

CK_DLL_SFUN(b2_body_set_bullet)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_SetBullet(body_id, GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(b2_body_is_bullet)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_IsBullet(body_id);
}

CK_DLL_SFUN(b2_body_enable_hit_events)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2Body_EnableHitEvents(body_id, GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(b2_body_get_shape_count)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_GetShapeCount(body_id);
}

// CK_DLL_SFUN(b2_body_get_shapes)
// {
//     b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
//     GET_NEXT_INT(ARGS); // advance
//     // TODO
// }

CK_DLL_SFUN(b2_body_get_joint_count)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_GetJointCount(body_id);
}

// CK_DLL_SFUN(b2_body_get_joints)
// {
//     b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
//     GET_NEXT_INT(ARGS); // advance
//     // TODO
// }

CK_DLL_SFUN(b2_body_get_contact_capacity)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    RETURN->v_int = b2Body_GetContactCapacity(body_id);
}

// CK_DLL_SFUN(b2_body_get_contact_data)
// {
//     b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
//     GET_NEXT_INT(ARGS); // advance
//     // TODO
// }

CK_DLL_SFUN(b2_body_compute_aabb)
{
    b2BodyId body_id = GET_B2_ID(b2BodyId, ARGS);
    GET_NEXT_INT(ARGS); // advance
    b2AABB box     = b2Body_ComputeAABB(body_id);
    RETURN->v_vec4 = { box.lowerBound.x, box.lowerBound.y, box.upperBound.x,
                       box.upperBound.y };
}

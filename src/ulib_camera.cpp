#include <chuck/chugin.h>

#include "sg_command.h"
#include "sg_component.h"

#include "ulib_helper.h"

#define GET_CAMERA(ckobj) SG_GetCamera(OBJ_MEMBER_UINT(ckobj, component_offset_id))

CK_DLL_CTOR(gcamera_ctor);

CK_DLL_MFUN(gcamera_set_mode_persp);
CK_DLL_MFUN(gcamera_set_mode_ortho);
CK_DLL_MFUN(gcamera_get_mode);

CK_DLL_MFUN(gcamera_set_clip);
CK_DLL_MFUN(gcamera_get_clip_near);
CK_DLL_MFUN(gcamera_get_clip_far);

// perspective camera params
// (no aspect, that's set automatically by renderer window resize callback)
CK_DLL_MFUN(gcamera_set_pers_fov);
CK_DLL_MFUN(gcamera_get_pers_fov);

// ortho camera params
CK_DLL_MFUN(gcamera_set_ortho_size); // view volume size (preserves screen aspect ratio)
CK_DLL_MFUN(gcamera_get_ortho_size);

CK_DLL_MFUN(gcamera_screen_coord_to_world_pos);
CK_DLL_MFUN(gcamera_world_pos_to_screen_coord);
CK_DLL_MFUN(gcamera_ndc_to_world_pos);
CK_DLL_MFUN(gcamera_world_pos_to_ndc);

// TODO overridable update(dt) (actualy don't we already get this from GGen?)
// add mouse click state to GWindow
// impl arcball Camera

static void ulib_camera_query(Chuck_DL_Query* QUERY)
{
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_CAMERA], SG_CKNames[SG_COMPONENT_TRANSFORM]);

    static t_CKINT camera_mode_persp = (t_CKINT)SG_CameraType_PERPSECTIVE;
    static t_CKINT camera_mode_ortho = (t_CKINT)SG_CameraType_ORTHOGRAPHIC;
    SVAR("int", "PERSPECTIVE", &camera_mode_persp);
    SVAR("int", "ORTHOGRAPHIC", &camera_mode_ortho);

    CTOR(gcamera_ctor);

    // camera mode
    MFUN(gcamera_set_mode_persp, "void", "perspective");
    DOC_FUNC("Set camera mode to perspective.");
    MFUN(gcamera_set_mode_ortho, "void", "orthographic");
    DOC_FUNC("Set camera mode to orthographic.");
    MFUN(gcamera_get_mode, "int", "mode");
    DOC_FUNC(
      "Get camera mode. Returns either GCamera.PERSPECTIVE or GCamera.ORTHOGRAPHIC.");

    // clip planes
    MFUN(gcamera_set_clip, "void", "clip");
    ARG("float", "near");
    ARG("float", "far");
    DOC_FUNC("Set camera clip planes.");

    MFUN(gcamera_get_clip_near, "float", "clipNear");
    DOC_FUNC("Get camera near clip plane.");

    MFUN(gcamera_get_clip_far, "float", "clipFar");
    DOC_FUNC("Get camera far clip plane.");

    // perspective camera params
    MFUN(gcamera_set_pers_fov, "void", "fov");
    ARG("float", "fov_radians");
    DOC_FUNC("Set camera field of view in radians.");

    MFUN(gcamera_get_pers_fov, "float", "fov");
    DOC_FUNC("Get camera field of view in radians.");

    // ortho camera params
    MFUN(gcamera_set_ortho_size, "void", "size");
    ARG("float", "size");
    DOC_FUNC(
      "(orthographic mode) set the height of the view volume in world space units. "
      "Width is automatically calculated based on aspect ratio.");

    MFUN(gcamera_get_ortho_size, "float", "size");
    DOC_FUNC(
      "(orthographic mode) get the height of the view volume in world space units.");

    // raycast
    MFUN(gcamera_screen_coord_to_world_pos, "vec3", "screenCoordToWorldPos");
    ARG("vec2", "screen_pos");
    ARG("float", "distance");
    DOC_FUNC(
      "Returns the world position of a point in screen space at a given distance from "
      "the camera. "
      "Useful in combination with GWindow.mousePos() for mouse picking.");

    MFUN(gcamera_world_pos_to_screen_coord, "vec2", "worldPosToScreenCoord");
    ARG("vec3", "world_pos");
    DOC_FUNC(
      "Get a screen coordinate from a world position by casting a ray from worldPos "
      "back to the camera and finding the intersection with the near clipping plane"
      "world_pos is a vec3 representing a world position."
      "Returns a vec2 screen coordinate."
      "Remember, screen coordinates have origin at the top-left corner of the window");

    MFUN(gcamera_ndc_to_world_pos, "vec3", "NDCToWorldPos");
    ARG("vec3", "clip_pos");
    DOC_FUNC(
      "Convert a point in clip space to world space. Clip space x and y should be in "
      "range [-1, 1], and z in the range [0, 1]. For x and y, 0 is the center of the "
      "screen. For z, 0 is the near clip plane and 1 is the far clip plane.");

    MFUN(gcamera_world_pos_to_ndc, "vec3", "worldPosToNDC");
    ARG("vec3", "world_pos");
    DOC_FUNC("Convert a point in world space to this camera's clip space.");

    END_CLASS();
}

CK_DLL_CTOR(gcamera_ctor)
{
    SG_CameraParams default_cam_params = {}; // passing direclty for now rather than
                                             // creating separate CameraParams struct
    SG_Camera* cam = SG_CreateCamera(SELF, default_cam_params);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = cam->id;
    CQ_PushCommand_CameraCreate(cam);
}

CK_DLL_MFUN(gcamera_set_mode_persp)
{
    SG_Camera* cam          = GET_CAMERA(SELF);
    cam->params.camera_type = SG_CameraType_PERPSECTIVE;

    CQ_PushCommand_CameraSetParams(cam);
}

CK_DLL_MFUN(gcamera_set_mode_ortho)
{
    SG_Camera* cam          = GET_CAMERA(SELF);
    cam->params.camera_type = SG_CameraType_ORTHOGRAPHIC;

    CQ_PushCommand_CameraSetParams(cam);
}

CK_DLL_MFUN(gcamera_get_mode)
{
    SG_Camera* cam = GET_CAMERA(SELF);
    RETURN->v_int  = (t_CKINT)cam->params.camera_type;
}

CK_DLL_MFUN(gcamera_set_clip)
{
    SG_Camera* cam         = GET_CAMERA(SELF);
    cam->params.near_plane = GET_NEXT_FLOAT(ARGS);
    cam->params.far_plane  = GET_NEXT_FLOAT(ARGS);

    CQ_PushCommand_CameraSetParams(cam);
}

CK_DLL_MFUN(gcamera_get_clip_near)
{
    SG_Camera* cam  = GET_CAMERA(SELF);
    RETURN->v_float = cam->params.near_plane;
}

CK_DLL_MFUN(gcamera_get_clip_far)
{
    SG_Camera* cam  = GET_CAMERA(SELF);
    RETURN->v_float = cam->params.far_plane;
}

CK_DLL_MFUN(gcamera_set_pers_fov)
{
    SG_Camera* cam          = GET_CAMERA(SELF);
    cam->params.fov_radians = GET_NEXT_FLOAT(ARGS);

    CQ_PushCommand_CameraSetParams(cam);
}

CK_DLL_MFUN(gcamera_get_pers_fov)
{
    SG_Camera* cam  = GET_CAMERA(SELF);
    RETURN->v_float = cam->params.fov_radians;
}

CK_DLL_MFUN(gcamera_set_ortho_size)
{
    SG_Camera* cam   = GET_CAMERA(SELF);
    cam->params.size = GET_NEXT_FLOAT(ARGS);

    CQ_PushCommand_CameraSetParams(cam);
}

CK_DLL_MFUN(gcamera_get_ortho_size)
{
    SG_Camera* cam  = GET_CAMERA(SELF);
    RETURN->v_float = cam->params.size;
}

CK_DLL_MFUN(gcamera_screen_coord_to_world_pos)
{
    SG_Camera* cam      = GET_CAMERA(SELF);
    t_CKVEC2 screen_pos = GET_NEXT_VEC2(ARGS);
    float distance      = GET_NEXT_FLOAT(ARGS);

    t_CKVEC2 windowSize = CHUGL_Window_WindowSize();
    int screenWidth     = windowSize.x;
    int screenHeight    = windowSize.y;
    float aspect        = (float)screenWidth / (float)screenHeight;

    if (cam->params.camera_type == SG_CameraType_ORTHOGRAPHIC) {
        // calculate camera frustrum size in world space
        float frustrum_height = cam->params.size;
        float frustrum_width  = frustrum_height * aspect;

        // convert from normalized mouse coords to view space coords
        // (we negate viewY so that 0,0 is bottom left instead of top left)
        float view_x = frustrum_width * (screen_pos.x / screenWidth - 0.5f);
        float view_y = -frustrum_height * (screen_pos.y / screenHeight - 0.5f);

        // convert from view space coords to world space coords
        glm::vec3 world_pos
          = SG_Transform::worldMatrix(cam) * glm::vec4(view_x, view_y, -distance, 1.0f);

        RETURN->v_vec3 = { world_pos.x, world_pos.y, world_pos.z };
        return;
    } else if (cam->params.camera_type
               == SG_CameraType_PERPSECTIVE) { // perspective camera
        glm::vec2 ndc = { (screen_pos.x / screenWidth) * 2.0f - 1.0f,
                          1.0f - (screen_pos.y / screenHeight) * 2.0f };

        // first convert to normalized device coordinates in range [-1, 1]
        glm::vec4 lRayEnd_NDC(ndc, 1.0, 1.0f); // The far plane maps to Z=1 in NDC

        glm::mat4 M = glm::inverse(SG_Camera::projectionMatrix(cam, aspect)
                                   * SG_Camera::viewMatrix(cam));

        glm::vec3 lRayStart_world = SG_Transform::worldPosition(cam);
        glm::vec4 lRayEnd_world   = M * lRayEnd_NDC;
        lRayEnd_world /= lRayEnd_world.w; // perspective divide

        glm::vec3 lRayDir_world
          = glm::normalize(glm::vec3(lRayEnd_world) - lRayStart_world);
        glm::vec3 world_pos = lRayStart_world + distance * lRayDir_world;
        RETURN->v_vec3      = { world_pos.x, world_pos.y, world_pos.z };
        return;
    }
    ASSERT(false); // unsupported camera type
}

CK_DLL_MFUN(gcamera_ndc_to_world_pos)
{
    SG_Camera* cam      = GET_CAMERA(SELF);
    t_CKVEC3 ndc_pos    = GET_NEXT_VEC3(ARGS);
    t_CKVEC2 windowSize = CHUGL_Window_WindowSize();
    float aspect        = (float)windowSize.x / (float)windowSize.y;

    /*
    Formula:
    clip_pos = P * V * world_pos
    world_pos = inv(P * V) * clip_pos
    */

    glm::mat4 view = SG_Camera::viewMatrix(cam);
    glm::mat4 proj = SG_Camera::projectionMatrix(cam, aspect);
    glm::vec4 world
      = glm::inverse(proj * view) * glm::vec4(ndc_pos.x, ndc_pos.y, ndc_pos.z, 1.0f);
    world /= world.w; // perspective divide

    RETURN->v_vec3 = { world.x, world.y, world.z };
}

CK_DLL_MFUN(gcamera_world_pos_to_ndc)
{
    SG_Camera* cam      = GET_CAMERA(SELF);
    t_CKVEC3 world_pos  = GET_NEXT_VEC3(ARGS);
    t_CKVEC2 windowSize = CHUGL_Window_WindowSize();
    float aspect        = (float)windowSize.x / (float)windowSize.y;

    /*
    Formula:
    clip_pos = P * V * world_pos
    */

    glm::mat4 view = SG_Camera::viewMatrix(cam);
    glm::mat4 proj = SG_Camera::projectionMatrix(cam, aspect);
    glm::vec4 clip
      = proj * view * glm::vec4(world_pos.x, world_pos.y, world_pos.z, 1.0f);
    glm::vec4 ndc = clip / clip.w;

    RETURN->v_vec3 = { ndc.x, ndc.y, ndc.z };
}

CK_DLL_MFUN(gcamera_world_pos_to_screen_coord)
{
    SG_Camera* cam    = GET_CAMERA(SELF);
    t_CKVEC3 worldPos = GET_NEXT_VEC3(ARGS);

    t_CKVEC2 windowSize = CHUGL_Window_WindowSize();
    float aspect        = windowSize.x / windowSize.y;

    // first convert to clip space
    glm::mat4 view = SG_Camera::viewMatrix(cam);

    glm::mat4 proj = SG_Camera::projectionMatrix(cam, aspect);
    glm::vec4 clipPos
      = proj * view * glm::vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f);

    // convert to screen space
    float x = (clipPos.x / clipPos.w + 1.0f) / 2.0f * windowSize.x;
    // need to invert y because screen coordinates are top-left origin
    float y = (1.0f - clipPos.y / clipPos.w) / 2.0f * windowSize.y;
    // z is depth value (buggy)
    // float z        = clipPos.z / clipPos.w;
    RETURN->v_vec2 = { x, y };
}

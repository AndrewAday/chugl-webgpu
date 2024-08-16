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
CK_DLL_MFUN(gcamera_set_pers_fov); // set in degrees // TODO make radians
CK_DLL_MFUN(gcamera_get_pers_fov);

// ortho camera params
CK_DLL_MFUN(gcamera_set_ortho_size); // view volume size (preserves screen aspect ratio)
CK_DLL_MFUN(gcamera_get_ortho_size);

// mouse cast from camera TODO
// CK_DLL_MFUN(chugl_cam_screen_coord_to_world_ray);
// CK_DLL_MFUN(chugl_cam_world_pos_to_screen_coord);

// TODO overridable update(dt) (actualy don't we already get this from GGen?)
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

    END_CLASS();
}

CK_DLL_CTOR(gcamera_ctor)
{
    SG_Camera default_cam = {}; // passing direclty for now rather than creating
                                // separate CameraParams struct
    SG_Camera* cam                             = SG_CreateCamera(SELF, &default_cam);
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

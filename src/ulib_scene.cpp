#include <chuck/chugin.h>

#include "sg_command.h"
#include "sg_component.h"

#include "ulib_helper.h"

CK_DLL_CTOR(gscene_ctor);
CK_DLL_DTOR(gscene_dtor);

CK_DLL_MFUN(gscene_set_background_color);
CK_DLL_MFUN(gscene_get_background_color);

CK_DLL_MFUN(gscene_set_main_camera);
CK_DLL_MFUN(gscene_get_main_camera);

static void ulib_gscene_query(Chuck_DL_Query* QUERY)
{
    // EM_log(CK_LOG_INFO, "ChuGL scene");
    // CGL scene
    QUERY->begin_class(QUERY, SG_CKNames[SG_COMPONENT_SCENE],
                       SG_CKNames[SG_COMPONENT_TRANSFORM]);
    QUERY->add_ctor(QUERY, gscene_ctor);

    // background color
    QUERY->add_mfun(QUERY, gscene_set_background_color, "vec4", "backgroundColor");
    QUERY->add_arg(QUERY, "vec4", "color");
    QUERY->doc_func(QUERY, "Set the background color of the scene");

    QUERY->add_mfun(QUERY, gscene_get_background_color, "vec4", "backgroundColor");
    QUERY->doc_func(QUERY, "Get the background color of the scene");

    // main camera
    MFUN(gscene_set_main_camera, "GCamera", "camera");
    ARG("GCamera", "camera");
    DOC_FUNC("Set the main camera of the scene");

    MFUN(gscene_get_main_camera, "GCamera", "camera");
    DOC_FUNC("Get the main camera of the scene");

    // end class -----------------------------------------------------
    QUERY->end_class(QUERY);
}

CK_DLL_CTOR(gscene_ctor)
{
    CQ_PushCommand_SceneCreate(SELF, component_offset_id, API);
}

CK_DLL_DTOR(gscene_dtor)
{
    // TODO
}

CK_DLL_MFUN(gscene_get_background_color)
{
    SG_Scene* scene = SG_GetScene(OBJ_MEMBER_UINT(SELF, component_offset_id));
    glm::vec4 color = scene->bg_color;
    RETURN->v_vec4  = { color.r, color.g, color.b, color.a };
}

CK_DLL_MFUN(gscene_set_background_color)
{
    SG_Scene* scene = SG_GetScene(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC4 color  = GET_NEXT_VEC4(ARGS);

    CQ_PushCommand_SceneBGColor(scene, color);

    RETURN->v_vec4 = color;
}

CK_DLL_MFUN(gscene_set_main_camera)
{
    SG_Scene* scene      = SG_GetScene(OBJ_MEMBER_UINT(SELF, component_offset_id));
    Chuck_Object* ck_cam = GET_NEXT_OBJECT(ARGS);

    SG_Camera* cam = ck_cam ? SG_GetCamera(OBJ_MEMBER_UINT(ck_cam, component_offset_id)) : NULL;

    if (cam && cam->id == scene->main_camera_id) {
        RETURN->v_object = ck_cam;
        return;
    }

    // check if camera is connected to scene
    if (cam && !SG_Transform::isAncestor(scene, cam)) {
        // CK_LOG(CK_LOG_CORE,
        //        "Warning: Remember to gruck the main camera to its scene!");
        CK_THROW("DisconnctedCamera",
                 "A camera must be connected (grucked) to scene before it can be set "
                 "as the main camera",
                 SHRED);
    }

    // refcount new camera
    SG_AddRef(cam);

    // deref old camera
    SG_DecrementRef(scene->main_camera_id);

    // update scene
    scene->main_camera_id = cam ? cam->id : 0;

    // update gfx thread
    CQ_PushCommand_SceneSetMainCamera(scene, cam);
}

CK_DLL_MFUN(gscene_get_main_camera)
{
    SG_Scene* scene = SG_GetScene(OBJ_MEMBER_UINT(SELF, component_offset_id));
    SG_Camera* cam  = SG_GetCamera(scene->main_camera_id);

    RETURN->v_object = cam ? cam->ckobj : NULL;
}

#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

// TODO add ambient to scene

CK_DLL_CTOR(ulib_light_ctor);

CK_DLL_MFUN(ulib_light_set_type);
CK_DLL_MFUN(ulib_light_get_type);

#define GET_LIGHT(ckobj) SG_GetLight(OBJ_MEMBER_UINT(ckobj, component_offset_id))

SG_Light* ulib_light_create(Chuck_Object* ckobj, SG_LightType type)
{
    CK_DL_API API = g_chuglAPI;

    // execute change on audio thread side
    SG_Light* light  = SG_CreateLight(ckobj);
    light->desc.type = type;
    // save SG_ID
    OBJ_MEMBER_UINT(ckobj, component_offset_id) = light->id;

    CQ_PushCommand_LightUpdate(light);
    return light;
}

static void ulib_light_query(Chuck_DL_Query* QUERY)
{
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_LIGHT], SG_CKNames[SG_COMPONENT_TRANSFORM]);

    static t_CKINT light_type_directional = SG_LightType_Directional;
    static t_CKINT light_type_point       = SG_LightType_Point;
    SVAR("int", "Directional", &light_type_directional);
    SVAR("int", "Point", &light_type_point);

    // -------------------------

    CTOR(ulib_light_ctor);

    MFUN(ulib_light_set_type, "void", "mode");
    ARG("int", "mode");
    DOC_FUNC("Set the light type. Use GLight.Directional or GLight.Point");

    MFUN(ulib_light_get_type, "int", "mode");
    DOC_FUNC("Get the light type. Returns either GLight.Directional or GLight.Point.");

    END_CLASS();
}

CK_DLL_CTOR(ulib_light_ctor)
{
    ulib_light_create(SELF, SG_LightType_Directional);
}

CK_DLL_MFUN(ulib_light_set_type)
{
    SG_Light* light  = GET_LIGHT(SELF);
    t_CKINT type     = GET_NEXT_INT(ARGS);
    light->desc.type = (SG_LightType)type;

    CQ_PushCommand_LightUpdate(light);
}

CK_DLL_MFUN(ulib_light_get_type)
{
    SG_Light* light = GET_LIGHT(SELF);
    RETURN->v_int   = (t_CKINT)light->desc.type;
}
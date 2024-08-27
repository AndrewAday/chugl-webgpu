#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "shaders.h"

#define GET_PASS(ckobj) SG_GetPass(OBJ_MEMBER_UINT(ckobj, component_offset_id))

// external API
SG_ID ulib_pass_createPass(SG_PassType pass_type);

// Pass
CK_DLL_CTOR(pass_ctor);
CK_DLL_MFUN(pass_get_next);
CK_DLL_GFUN(pass_op_gruck);   // add pass
CK_DLL_GFUN(pass_op_ungruck); // remove pass

// RenderPass
CK_DLL_CTOR(renderpass_ctor);
CK_DLL_MFUN(renderpass_set_resolve_target);
CK_DLL_MFUN(renderpass_get_resolve_target);
// TODO get_resolve_target
// TODO set/get camera and scene
// - enforce camera is part of scene
// TODO add RenderPassColorAttachment.loadOp / storeOp / clearValue (clearValue in
// GScene)
// TODO add set/get HDR?
// TODO add set/get MSAA sample count?
// TODO scissor + viewport (after ScreenPass, not sure if we apply scissor/viewport to
// RenderPass or screenpass...)

// Other
// WindowTexture

const char* ulib_pass_classname(SG_PassType pass_type)
{
    switch (pass_type) {
        case SG_PassType_Root: return SG_CKNames[SG_COMPONENT_PASS];
        case SG_PassType_Render: return "RenderPass";
        case SG_PassType_Compute: return "ComputePass";
        case SG_PassType_Screen: return "ScreenPass";
        default: ASSERT(false);
    }
    return NULL;
}

void ulib_pass_query(Chuck_DL_Query* QUERY)
{
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_PASS], SG_CKNames[SG_COMPONENT_BASE]);

    CTOR(pass_ctor);

    MFUN(pass_get_next, SG_CKNames[SG_COMPONENT_PASS], "next");
    DOC_FUNC("Get the GPass this is connected to");

    QUERY->add_op_overload_binary(QUERY, pass_op_gruck, SG_CKNames[SG_COMPONENT_PASS],
                                  "-->", SG_CKNames[SG_COMPONENT_PASS], "lhs",
                                  SG_CKNames[SG_COMPONENT_PASS], "rhs");

    // overload GGen --< GGen
    QUERY->add_op_overload_binary(QUERY, pass_op_ungruck, SG_CKNames[SG_COMPONENT_PASS],
                                  "--<", SG_CKNames[SG_COMPONENT_PASS], "lhs",
                                  SG_CKNames[SG_COMPONENT_PASS], "rhs");
    END_CLASS();

    // RenderPass --------------------------------------------------------------
    // TODO: rename to RenderPass?
    BEGIN_CLASS(ulib_pass_classname(SG_PassType_Render), SG_CKNames[SG_COMPONENT_PASS]);
    DOC_CLASS(
      " Render pass for drawing a GScene. If RenderPass.scene() is not set, will "
      "default to the main scene, GG.scene()"
      " If RenderPass.target() is not set, will default to the screen. "
      " If RenderPass.camera() is not set, will default to the scene's main camera: "
      "GG.scene().camera()");

    CTOR(renderpass_ctor);

    MFUN(renderpass_set_resolve_target, "void", "target");
    ARG(SG_CKNames[SG_COMPONENT_TEXTURE], "target");
    DOC_FUNC("Set the target texture to draw the scene to.");

    MFUN(renderpass_get_resolve_target, SG_CKNames[SG_COMPONENT_TEXTURE], "target");
    DOC_FUNC("Get the target texture to draw the scene to.");

    END_CLASS();
}

SG_ID ulib_pass_createPass(SG_PassType pass_type)
{
    CK_DL_API API            = g_chuglAPI;
    Chuck_Object* pass_ckobj = chugin_createCkObj(ulib_pass_classname(pass_type), true);

    SG_Pass* pass = SG_CreatePass(pass_ckobj, pass_type);
    ASSERT(pass->pass_type == pass_type);

    OBJ_MEMBER_UINT(pass_ckobj, component_offset_id) = pass->id;

    CQ_PushCommand_PassUpdate(pass);

    return pass->id;
}

CK_DLL_CTOR(pass_ctor)
{
    // don't allow instantiation of abstract base class
    if (chugin_typeEquals(SELF, SG_CKNames[SG_COMPONENT_PASS])) {
        CK_THROW(
          "InvalidClassInstantiation",
          "GPass is an abstract base class, do not instantiate directly. Use one of "
          "the children classes e.g. RenderPass / ComputePass / ScreenPass instead",
          SHRED);
    }
}

CK_DLL_MFUN(pass_get_next)
{
    SG_Pass* pass      = GET_PASS(SELF);
    SG_Pass* next_pass = SG_GetPass(pass->next_pass_id);

    RETURN->v_object = next_pass ? next_pass->ckobj : NULL;
}

CK_DLL_GFUN(pass_op_gruck)
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
    SG_Pass* lhs_pass = SG_GetPass(OBJ_MEMBER_UINT(lhs, component_offset_id));
    SG_Pass* rhs_pass = SG_GetPass(OBJ_MEMBER_UINT(rhs, component_offset_id));

    if (!SG_Pass::connect(lhs_pass, rhs_pass)) {
        CK_LOG(CK_LOG_WARNING,
               "warning GPass --> GPass failed! Cannot connect NULL passes, cannot "
               "form cycles in the GPass chain");
    }

    // command
    CQ_PushCommand_PassUpdate(lhs_pass);

    // return RHS
    RETURN->v_object = rhs;
}

CK_DLL_GFUN(pass_op_ungruck)
{
    // get the arguments
    Chuck_Object* lhs = GET_NEXT_OBJECT(ARGS);
    Chuck_Object* rhs = GET_NEXT_OBJECT(ARGS);

    // get internal representation
    SG_Pass* lhs_pass = SG_GetPass(OBJ_MEMBER_UINT(lhs, component_offset_id));
    SG_Pass* rhs_pass = SG_GetPass(OBJ_MEMBER_UINT(rhs, component_offset_id));

    SG_Pass::disconnect(lhs_pass, rhs_pass);

    // command
    CQ_PushCommand_PassUpdate(lhs_pass);

    // return RHS
    RETURN->v_object = rhs;
}

// ============================================================================
// RenderPass
// ============================================================================

CK_DLL_CTOR(renderpass_ctor)
{
    SG_Pass* pass = SG_CreatePass(SELF, SG_PassType_Render);
    ASSERT(pass->type == SG_COMPONENT_PASS);
    ASSERT(pass->pass_type == SG_PassType_Render);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = pass->id;

    CQ_PushCommand_PassUpdate(pass);
}

CK_DLL_MFUN(renderpass_set_resolve_target)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Render);

    Chuck_Object* target = GET_NEXT_OBJECT(ARGS);
    SG_Texture* texture
      = target ? SG_GetTexture(OBJ_MEMBER_UINT(target, component_offset_id)) : NULL;
    SG_Pass::resolveTarget(pass, texture);

    // command TODO
}

CK_DLL_MFUN(renderpass_get_resolve_target)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Render);

    SG_Texture* texture = SG_GetTexture(pass->resolve_target_id);
    RETURN->v_object    = texture ? texture->ckobj : NULL;
}
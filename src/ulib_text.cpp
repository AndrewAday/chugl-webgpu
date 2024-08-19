#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "shaders.h"

// TODO
// g_material_builtin_shaders.gtext_shader_id

CK_DLL_CTOR(gtext_ctor);

void ulib_text_query(Chuck_DL_Query* QUERY)
{
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_TEXT], SG_CKNames[SG_COMPONENT_TRANSFORM]);
    CTOR(gtext_ctor);
    END_CLASS();
}

CK_DLL_CTOR(gtext_ctor)
{
    SG_Text* text = SG_CreateText(SELF);
    ASSERT(text->type == SG_COMPONENT_TEXT);

    OBJ_MEMBER_UINT(SELF, component_offset_id) = text->id;

    text->text = "hello world";

    CQ_PushCommand_TextCreate(text, g_material_builtin_shaders.gtext_shader_id);

    // @group(1) @binding(2) var<uniform> u_Color: vec4f;
    // @group(1) @binding(3) var<uniform> antiAliasingWindowSize: f32 = 1.0;
    // @group(1) @binding(4) var<uniform> enableSuperSamplingAntiAliasing: i32 = 1;

    // set uniform
    // TODO store on sg_text struct later
    // TODO where to store default uniform values + locations?
    // SG_Material::uniformFloat(material, 0, 0.1f); // thickness
    // TODO extrusion uniform

    // CQ_PushCommand_MaterialSetUniform(material, 0);
}
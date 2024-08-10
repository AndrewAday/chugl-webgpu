#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#define GET_SHADER(ckobj) SG_GetShader(OBJ_MEMBER_UINT(ckobj, component_offset_id))
#define GET_MATERIAL(ckobj) SG_GetMaterial(OBJ_MEMBER_UINT(ckobj, component_offset_id))

CK_DLL_CTOR(shader_desc_ctor);
static t_CKUINT shader_desc_vertex_string_offset     = 0;
static t_CKUINT shader_desc_fragment_string_offset   = 0;
static t_CKUINT shader_desc_vertex_filepath_offset   = 0;
static t_CKUINT shader_desc_fragment_filepath_offset = 0;
static t_CKUINT shader_desc_vertex_layout_offset     = 0;

CK_DLL_CTOR(shader_ctor_default);
CK_DLL_CTOR(shader_ctor);
CK_DLL_MFUN(shader_get_vertex_string);
CK_DLL_MFUN(shader_get_fragment_string);
CK_DLL_MFUN(shader_get_vertex_filepath);
CK_DLL_MFUN(shader_get_fragment_filepath);
CK_DLL_MFUN(shader_get_vertex_layout);

CK_DLL_CTOR(material_ctor);

// material pso
CK_DLL_MFUN(material_get_shader);
CK_DLL_MFUN(material_set_shader);
CK_DLL_MFUN(material_get_cullmode);
CK_DLL_MFUN(material_set_cullmode);

CK_DLL_MFUN(material_uniform_remove);
CK_DLL_MFUN(material_uniform_active_locations);

CK_DLL_MFUN(material_set_uniform_float);
CK_DLL_MFUN(material_get_uniform_float);

CK_DLL_CTOR(pbr_material_ctor);

void ulib_material_query(Chuck_DL_Query* QUERY)
{
    // TODO today: documentation

    // ShaderDesc -----------------------------------------------------
    BEGIN_CLASS("ShaderDesc", "Object");
    DOC_CLASS(
      "Shader description object. Used to create a Shader component."
      "Set either vertexString or vertexFilepath, and either fragmentString or "
      "fragmentFilepath.");

    CTOR(shader_desc_ctor);

    shader_desc_vertex_string_offset     = MVAR("string", "vertexString", false);
    shader_desc_fragment_string_offset   = MVAR("string", "fragmentString", false);
    shader_desc_vertex_filepath_offset   = MVAR("string", "vertexFilepath", false);
    shader_desc_fragment_filepath_offset = MVAR("string", "fragmentFilepath", false);
    shader_desc_vertex_layout_offset     = MVAR("int[]", "vertexLayout", false);

    END_CLASS();

    // Shader -----------------------------------------------------
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_SHADER], SG_CKNames[SG_COMPONENT_BASE]);

    CTOR(shader_ctor_default);

    CTOR(shader_ctor);
    ARG("ShaderDesc", "shader_desc");
    DOC_FUNC("Create a Shader component. Immutable.");

    MFUN(shader_get_vertex_string, "string", "vertexString");
    DOC_FUNC("Get the vertex shader string passed in the ShaderDesc at creation.");

    MFUN(shader_get_fragment_string, "string", "fragmentString");
    DOC_FUNC("Get the fragment shader string passed in the ShaderDesc at creation.");

    MFUN(shader_get_vertex_filepath, "string", "vertexFilepath");
    DOC_FUNC("Get the vertex shader filepath passed in the ShaderDesc at creation.");

    MFUN(shader_get_fragment_filepath, "string", "fragmentFilepath");
    DOC_FUNC("Get the fragment shader filepath passed in the ShaderDesc at creation.");

    MFUN(shader_get_vertex_layout, "int[]", "vertexLayout");
    DOC_FUNC("Get the vertex layout passed in the ShaderDesc at creation.");

    END_CLASS();

    // Material -----------------------------------------------------
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_MATERIAL], SG_CKNames[SG_COMPONENT_BASE]);

    CTOR(material_ctor);

    // svars
    static t_CKINT cullmode_none  = WGPUCullMode_None;
    static t_CKINT cullmode_front = WGPUCullMode_Front;
    static t_CKINT cullmode_back  = WGPUCullMode_Back;
    SVAR("int", "CULL_NONE", &cullmode_none);
    DOC_VAR("No culling.");
    SVAR("int", "CULL_FRONT", &cullmode_front);
    DOC_VAR("Cull front faces.");
    SVAR("int", "CULL_BACK", &cullmode_back);
    DOC_VAR("Cull back faces.");

    // pso modifiers (shouldn't be set often, so we lump all together in a single
    // command that copies the entire PSO struct)
    MFUN(material_get_shader, SG_CKNames[SG_COMPONENT_SHADER], "shader");

    MFUN(material_set_shader, "void", "shader");
    ARG(SG_CKNames[SG_COMPONENT_SHADER], "shader");

    MFUN(material_get_cullmode, "int", "cullMode");
    DOC_FUNC(
      "Get the cull mode of the material. Material.CULL_NONE, Material.CULL_FRONT, or "
      "Material.CULL_BACK.");

    MFUN(material_set_cullmode, "void", "cullMode");
    ARG("int", "cullMode");
    DOC_FUNC(
      "Set the cull mode of the material. valid options: Material.CULL_NONE, "
      "Material.CULL_FRONT, or Material.CULL_BACK.");

    // uniforms
    MFUN(material_uniform_remove, "void", "removeUniform");
    ARG("int", "location");

    MFUN(material_uniform_active_locations, "int[]", "activeUniformLocations");

    MFUN(material_set_uniform_float, "void", "uniformFloat");
    ARG("int", "location");
    ARG("float", "uniform_value");

    MFUN(material_get_uniform_float, "float", "uniformFloat");
    ARG("int", "location");

    // abstract class, no destructor or constructor
    END_CLASS();

    // PBR Material -----------------------------------------------------
    BEGIN_CLASS("PBRMaterial", SG_CKNames[SG_COMPONENT_MATERIAL]);

    CTOR(pbr_material_ctor);

    // abstract class, no destructor or constructor
    END_CLASS();
}

// Shader ===================================================================

CK_DLL_CTOR(shader_desc_ctor)
{
    // TODO today: should we default vertex layout to [3, 3, 2, 4]?

    // chuck doesn't initialize class member vars in constructor. create manually
    // create string members (empty string)
    OBJ_MEMBER_STRING(SELF, shader_desc_vertex_string_offset)
      = chugin_createCkString("");
    OBJ_MEMBER_STRING(SELF, shader_desc_fragment_string_offset)
      = chugin_createCkString("");
    OBJ_MEMBER_STRING(SELF, shader_desc_vertex_filepath_offset)
      = chugin_createCkString("");
    OBJ_MEMBER_STRING(SELF, shader_desc_fragment_filepath_offset)
      = chugin_createCkString("");
}

CK_DLL_CTOR(shader_ctor_default)
{
    CK_THROW("ShaderConstructor",
             "Shader default constructor not allowed. use Shader(ShaderDesc) instead",
             SHRED);
}

CK_DLL_CTOR(shader_ctor)
{
    Chuck_Object* shader_desc = GET_NEXT_OBJECT(ARGS);

    // create shader on audio side
    SG_Shader* shader = SG_CreateShader(
      SELF, OBJ_MEMBER_STRING(shader_desc, shader_desc_vertex_string_offset),
      OBJ_MEMBER_STRING(shader_desc, shader_desc_fragment_string_offset),
      OBJ_MEMBER_STRING(shader_desc, shader_desc_vertex_filepath_offset),
      OBJ_MEMBER_STRING(shader_desc, shader_desc_fragment_filepath_offset),
      OBJ_MEMBER_INT_ARRAY(shader_desc, shader_desc_vertex_layout_offset));

    // save component id
    OBJ_MEMBER_UINT(SELF, component_offset_id) = shader->id;

    // push to command queue
    CQ_PushCommand_ShaderCreate(shader);
}

CK_DLL_MFUN(shader_get_vertex_string)
{
    SG_Shader* shader = GET_SHADER(SELF);
    RETURN->v_string  = chugin_createCkString(shader->vertex_string_owned);
}

CK_DLL_MFUN(shader_get_fragment_string)
{
    SG_Shader* shader = GET_SHADER(SELF);
    RETURN->v_string  = chugin_createCkString(shader->fragment_string_owned);
}

CK_DLL_MFUN(shader_get_vertex_filepath)
{
    SG_Shader* shader = GET_SHADER(SELF);
    RETURN->v_string  = chugin_createCkString(shader->vertex_filepath_owned);
}

CK_DLL_MFUN(shader_get_fragment_filepath)
{
    SG_Shader* shader = GET_SHADER(SELF);
    RETURN->v_string  = chugin_createCkString(shader->fragment_filepath_owned);
}

CK_DLL_MFUN(shader_get_vertex_layout)
{
    SG_Shader* shader = GET_SHADER(SELF);
    RETURN->v_object  = (Chuck_Object*)chugin_createCkIntArray(
      shader->vertex_layout, ARRAY_LENGTH(shader->vertex_layout));
}

// Material ===================================================================

CK_DLL_CTOR(material_ctor)
{
    SG_Material* material = SG_CreateMaterial(SELF, SG_MATERIAL_CUSTOM, NULL);
    ASSERT(material->type == SG_COMPONENT_MATERIAL);

    OBJ_MEMBER_UINT(SELF, component_offset_id) = material->id;

    CQ_PushCommand_MaterialCreate(material);
}

CK_DLL_MFUN(material_get_shader)
{
    SG_Material* material = GET_MATERIAL(SELF);
    SG_Shader* shader     = SG_GetShader(material->pso.sg_shader_id);
    RETURN->v_object      = shader ? shader->ckobj : NULL;
}

CK_DLL_MFUN(material_set_shader)
{
    Chuck_Object* shader  = GET_NEXT_OBJECT(ARGS);
    SG_Material* material = GET_MATERIAL(SELF);

    // set shader on audio side
    SG_Material::shader(material, GET_SHADER(shader));

    // push to command queue
    CQ_PushCommand_MaterialUpdatePSO(material);
}

CK_DLL_MFUN(material_get_cullmode)
{
    SG_Material* material = GET_MATERIAL(SELF);
    RETURN->v_int         = (t_CKINT)material->pso.cull_mode;
}

CK_DLL_MFUN(material_set_cullmode)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    t_CKINT cull_mode       = GET_NEXT_INT(ARGS);
    material->pso.cull_mode = (WGPUCullMode)cull_mode;

    CQ_PushCommand_MaterialUpdatePSO(material);
}

CK_DLL_MFUN(material_uniform_active_locations)
{
    SG_Material* material = GET_MATERIAL(SELF);

    int active_locations[SG_MATERIAL_MAX_UNIFORMS];
    int active_locations_count = 0;

    for (int i = 0; i < SG_MATERIAL_MAX_UNIFORMS; i++) {
        if (material->uniforms[i].type != SG_MATERIAL_UNIFORM_NONE) {
            active_locations[active_locations_count++] = i;
        }
    }

    RETURN->v_object = (Chuck_Object*)chugin_createCkIntArray(active_locations,
                                                              active_locations_count);
}

CK_DLL_MFUN(material_uniform_remove)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    SG_Material::removeUniform(material, location);

    // TODO push to command queue
}

CK_DLL_MFUN(material_set_uniform_float)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    t_CKINT location        = GET_NEXT_INT(ARGS);
    t_CKFLOAT uniform_value = GET_NEXT_FLOAT(ARGS);

    SG_Material::uniformFloat(material, location, (f32)uniform_value);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_float)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_FLOAT) {
        CK_THROW("MaterialGetUniformFloat", "Uniform location is not a float", SHRED);
    }

    RETURN->v_float = (t_CKFLOAT)SG_Material::uniformFloat(material, location);
}

CK_DLL_CTOR(pbr_material_ctor)
{
    // SG_Material* material = GET_MATERIAL(SELF);
    // ASSERT(material->type == SG_COMPONENT_MATERIAL);
    // ASSERT(material->material_type == SG_MATERIAL_PBR);

    // OBJ_MEMBER_UINT(SELF, component_offset_id) = material->id;

    // CQ_PushCommand_MaterialCreate(material);
}
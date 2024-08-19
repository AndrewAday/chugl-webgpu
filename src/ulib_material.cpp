#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "shaders.h"

#define GET_SHADER(ckobj) SG_GetShader(OBJ_MEMBER_UINT(ckobj, component_offset_id))
#define GET_MATERIAL(ckobj) SG_GetMaterial(OBJ_MEMBER_UINT(ckobj, component_offset_id))

void chugl_initDefaultMaterials();

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
CK_DLL_MFUN(material_set_topology);
CK_DLL_MFUN(material_get_topology);

// material uniforms
CK_DLL_MFUN(material_uniform_remove);
CK_DLL_MFUN(material_uniform_active_locations);

CK_DLL_MFUN(material_set_uniform_float);
CK_DLL_MFUN(material_get_uniform_float);
CK_DLL_MFUN(material_set_uniform_float2);
CK_DLL_MFUN(material_get_uniform_float2);
CK_DLL_MFUN(material_set_uniform_float3);
CK_DLL_MFUN(material_get_uniform_float3);
CK_DLL_MFUN(material_set_uniform_float4);
CK_DLL_MFUN(material_get_uniform_float4);

CK_DLL_MFUN(material_set_uniform_int);
CK_DLL_MFUN(material_get_uniform_int);

// getting back storage buffers is tricky because it may have been modified by shader
// so no getter for now (until we figure out compute shaders)
/*
possible impl for storage buffer getter:
- StorageBufferEvent to handle async buffer map from gpu --> render thread --> audio
thread
- GPUBuffer component exposed through chugl API that can be queried for buffer data
    - setStorageBuffer() instead of taking a ck_FloatArray, takes a GPUBuffer component
*/
CK_DLL_MFUN(material_set_storage_buffer);
CK_DLL_MFUN(material_set_sampler);
CK_DLL_MFUN(material_set_texture);

CK_DLL_CTOR(lines2d_material_ctor);
CK_DLL_MFUN(lines2d_material_get_thickness);
CK_DLL_MFUN(lines2d_material_set_thickness);

CK_DLL_CTOR(flat_material_ctor);
CK_DLL_MFUN(flat_material_get_color);
CK_DLL_MFUN(flat_material_set_color);

CK_DLL_CTOR(pbr_material_ctor);

void ulib_material_query(Chuck_DL_Query* QUERY)
{
    // TODO today: documentation

    BEGIN_CLASS("VertexFormat", "Object");
    DOC_CLASS("Vertex format enum. Used to describe vertex data layout in ShaderDesc.");

    static t_CKINT format_float  = WGPUVertexFormat_Float32;
    static t_CKINT format_float2 = WGPUVertexFormat_Float32x2;
    static t_CKINT format_float3 = WGPUVertexFormat_Float32x3;
    static t_CKINT format_float4 = WGPUVertexFormat_Float32x4;
    static t_CKINT format_int    = WGPUVertexFormat_Sint32;
    static t_CKINT format_int2   = WGPUVertexFormat_Sint32x2;
    static t_CKINT format_int3   = WGPUVertexFormat_Sint32x3;
    static t_CKINT format_int4   = WGPUVertexFormat_Sint32x4;

    SVAR("int", "FLOAT", &format_float);
    SVAR("int", "FLOAT2", &format_float2);
    SVAR("int", "FLOAT3", &format_float3);
    SVAR("int", "FLOAT4", &format_float4);
    SVAR("int", "INT", &format_int);
    SVAR("int", "INT2", &format_int2);
    SVAR("int", "INT3", &format_int3);
    SVAR("int", "INT4", &format_int4);

    END_CLASS();

    // ShaderDesc -----------------------------------------------------
    BEGIN_CLASS("ShaderDesc", "Object");
    DOC_CLASS(
      "Shader description object. Used to create a Shader component."
      "Set either vertexString or vertexFilepath, and either fragmentString or "
      "fragmentFilepath. `vertexLayout` field describes the vertex data layout "
      "of buffers going into the vertex shader--use the VertexFormat enum."
      "E.g. if your vertex shader takes a vec3 position and a vec2 uv, set "
      "`vertexLayout` to [VertexFormat.FLOAT3, VertexFormat.FLOAT2].");

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

    static t_CKINT topology_pointlist     = WGPUPrimitiveTopology_PointList;
    static t_CKINT topology_linelist      = WGPUPrimitiveTopology_LineList;
    static t_CKINT topology_linestrip     = WGPUPrimitiveTopology_LineStrip;
    static t_CKINT topology_trianglelist  = WGPUPrimitiveTopology_TriangleList;
    static t_CKINT topology_trianglestrip = WGPUPrimitiveTopology_TriangleStrip;
    SVAR("int", "TOPOLOGY_POINTLIST", &topology_pointlist);
    DOC_VAR("Interpret each vertex as a point.");
    SVAR("int", "TOPOLOGY_LINELIST", &topology_linelist);
    DOC_VAR("Interpret each pair of vertices as a line.");
    SVAR("int", "TOPOLOGY_LINESTRIP", &topology_linestrip);
    DOC_VAR(
      "Each vertex after the first defines a line primitive between it and the "
      "previous vertex.");
    SVAR("int", "TOPOLOGY_TRIANGLELIST", &topology_trianglelist);
    DOC_VAR("Interpret each triplet of vertices as a triangle.");
    SVAR("int", "TOPOLOGY_TRIANGLESTRIP", &topology_trianglestrip);
    DOC_VAR(
      "Each vertex after the first two defines a triangle primitive between it and the "
      "previous two vertices.");

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

    MFUN(material_set_topology, "void", "topology");
    ARG("int", "topology");
    DOC_FUNC(
      "Set the primitive topology of the material. valid options: "
      "Material.TOPOLOGY_POINTLIST, Material.TOPOLOGY_LINELIST, "
      "Material.TOPOLOGY_LINESTRIP, Material.TOPOLOGY_TRIANGLELIST, or "
      "Material.TOPOLOGY_TRIANGLESTRIP.");

    MFUN(material_get_topology, "int", "topology");
    DOC_FUNC(
      "Get the primitive topology of the material. Material.TOPOLOGY_POINTLIST, "
      "Material.TOPOLOGY_LINELIST, Material.TOPOLOGY_LINESTRIP, "
      "Material.TOPOLOGY_TRIANGLELIST, or Material.TOPOLOGY_TRIANGLESTRIP.");

    // uniforms
    MFUN(material_uniform_remove, "void", "removeUniform");
    ARG("int", "location");

    MFUN(material_uniform_active_locations, "int[]", "activeUniformLocations");

    MFUN(material_set_uniform_float, "void", "uniformFloat");
    ARG("int", "location");
    ARG("float", "uniform_value");

    MFUN(material_get_uniform_float, "float", "uniformFloat");
    ARG("int", "location");

    MFUN(material_set_uniform_int, "void", "uniformInt");
    ARG("int", "location");
    ARG("int", "uniform_value");

    MFUN(material_get_uniform_int, "int", "uniformInt");
    ARG("int", "location");

    MFUN(material_set_uniform_float2, "void", "uniformVec2");
    ARG("int", "location");
    ARG("vec2", "uniform_value");

    MFUN(material_get_uniform_float2, "vec2", "uniformVec2");
    ARG("int", "location");

    MFUN(material_set_uniform_float3, "void", "uniformVec3");
    ARG("int", "location");
    ARG("vec3", "uniform_value");

    MFUN(material_get_uniform_float3, "vec3", "uniformVec3");
    ARG("int", "location");

    MFUN(material_set_uniform_float4, "void", "uniformVec4");
    ARG("int", "location");
    ARG("vec4", "uniform_value");

    MFUN(material_get_uniform_float4, "vec4", "uniformVec4");
    ARG("int", "location");

    // storage buffers
    MFUN(material_set_storage_buffer, "void", "storageBuffer");
    ARG("int", "location");
    ARG("float[]", "storageBuffer");

    MFUN(material_set_sampler, "void", "sampler");
    ARG("int", "location");
    ARG("TextureSampler", "sampler");

    MFUN(material_set_texture, "void", "texture");
    ARG("int", "location");
    ARG("Texture", "texture");

    // abstract class, no destructor or constructor
    END_CLASS();

    // Lines2DMaterial -----------------------------------------------------
    BEGIN_CLASS("Lines2DMaterial", SG_CKNames[SG_COMPONENT_MATERIAL]);

    CTOR(lines2d_material_ctor);

    // thickness uniform
    MFUN(lines2d_material_get_thickness, "float", "thickness");
    DOC_FUNC("Get the thickness of the lines in the material.");

    MFUN(lines2d_material_set_thickness, "void", "thickness");
    ARG("float", "thickness");
    DOC_FUNC("Set the thickness of the lines in the material.");

    END_CLASS();

    // FlatMaterial -----------------------------------------------------

    BEGIN_CLASS("FlatMaterial", SG_CKNames[SG_COMPONENT_MATERIAL]);

    CTOR(flat_material_ctor);

    // color uniform
    MFUN(flat_material_get_color, "vec4", "color");
    DOC_FUNC("Get the color of the material.");

    MFUN(flat_material_set_color, "void", "color");
    ARG("vec4", "color");
    DOC_FUNC("Set the color of the material.");

    END_CLASS();

    // PBR Material -----------------------------------------------------
    BEGIN_CLASS("PBRMaterial", SG_CKNames[SG_COMPONENT_MATERIAL]);

    CTOR(pbr_material_ctor);

    // abstract class, no destructor or constructor
    END_CLASS();

    // initialize default components
    chugl_initDefaultMaterials();
}

// Shader ===================================================================

CK_DLL_CTOR(shader_desc_ctor)
{
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

static void chugl_materialSetShader(SG_Material* material, SG_Shader* shader)
{
    SG_Material::shader(material, shader);
    CQ_PushCommand_MaterialUpdatePSO(material);
}

CK_DLL_MFUN(material_set_shader)
{
    Chuck_Object* shader  = GET_NEXT_OBJECT(ARGS);
    SG_Material* material = GET_MATERIAL(SELF);

    chugl_materialSetShader(material, GET_SHADER(shader));
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

CK_DLL_MFUN(material_set_topology)
{
    SG_Material* material            = GET_MATERIAL(SELF);
    t_CKINT primitive_topology       = GET_NEXT_INT(ARGS);
    material->pso.primitive_topology = (WGPUPrimitiveTopology)primitive_topology;

    CQ_PushCommand_MaterialUpdatePSO(material);
}

CK_DLL_MFUN(material_get_topology)
{
    SG_Material* material = GET_MATERIAL(SELF);
    RETURN->v_int         = (t_CKINT)material->pso.primitive_topology;
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
    float uniform_value_f32 = (float)uniform_value;

    SG_Material::setUniform(material, location, &uniform_value_f32,
                            SG_MATERIAL_UNIFORM_FLOAT);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_float)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_FLOAT) {
        CK_THROW("MaterialGetUniformFloat", "Uniform location is not a float", SHRED);
    }

    RETURN->v_float = material->uniforms[location].as.f;
}

CK_DLL_MFUN(material_set_uniform_float2)
{
    SG_Material* material       = GET_MATERIAL(SELF);
    t_CKINT location            = GET_NEXT_INT(ARGS);
    t_CKVEC2 uniform_value      = GET_NEXT_VEC2(ARGS);
    glm::vec2 uniform_value_f32 = { uniform_value.x, uniform_value.y };

    SG_Material::setUniform(material, location, &uniform_value_f32,
                            SG_MATERIAL_UNIFORM_VEC2F);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_float2)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_VEC2F) {
        CK_THROW("MaterialGetUniformFloat2", "Uniform location is not a vec2", SHRED);
    }

    glm::vec2 uniform_value = material->uniforms[location].as.vec2f;
    RETURN->v_vec2          = { uniform_value.x, uniform_value.y };
}

CK_DLL_MFUN(material_set_uniform_float3)
{
    SG_Material* material       = GET_MATERIAL(SELF);
    t_CKINT location            = GET_NEXT_INT(ARGS);
    t_CKVEC3 uniform_value      = GET_NEXT_VEC3(ARGS);
    glm::vec3 uniform_value_f32 = { uniform_value.x, uniform_value.y, uniform_value.z };

    SG_Material::setUniform(material, location, &uniform_value_f32,
                            SG_MATERIAL_UNIFORM_VEC3F);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_float3)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_VEC3F) {
        CK_THROW("MaterialGetUniformFloat3", "Uniform location is not a vec3", SHRED);
    }

    glm::vec3 uniform_value = material->uniforms[location].as.vec3f;
    RETURN->v_vec3          = { uniform_value.x, uniform_value.y, uniform_value.z };
}

CK_DLL_MFUN(material_set_uniform_float4)
{
    SG_Material* material  = GET_MATERIAL(SELF);
    t_CKINT location       = GET_NEXT_INT(ARGS);
    t_CKVEC4 uniform_value = GET_NEXT_VEC4(ARGS);
    glm::vec4 uniform_value_f32
      = { uniform_value.x, uniform_value.y, uniform_value.z, uniform_value.w };

    SG_Material::setUniform(material, location, &uniform_value_f32,
                            SG_MATERIAL_UNIFORM_VEC4F);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_float4)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_VEC4F) {
        CK_THROW("MaterialGetUniformFloat4", "Uniform location is not a vec4", SHRED);
    }

    glm::vec4 uniform_value = material->uniforms[location].as.vec4f;
    RETURN->v_vec4
      = { uniform_value.x, uniform_value.y, uniform_value.z, uniform_value.w };
}

CK_DLL_MFUN(material_set_uniform_int)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);
    t_CKINT uniform_value = GET_NEXT_INT(ARGS);
    i32 uniform_value_i32 = (i32)uniform_value;

    SG_Material::setUniform(material, location, &uniform_value_i32,
                            SG_MATERIAL_UNIFORM_INT);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_int)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_INT) {
        CK_THROW("MaterialGetUniformInt", "Uniform location is not an int", SHRED);
    }

    RETURN->v_int = material->uniforms[location].as.i;
}

CK_DLL_MFUN(material_set_storage_buffer)
{
    SG_Material* material    = GET_MATERIAL(SELF);
    t_CKINT location         = GET_NEXT_INT(ARGS);
    Chuck_ArrayFloat* ck_arr = GET_NEXT_FLOAT_ARRAY(ARGS);

    SG_Material::setStorageBuffer(material, location);

    CQ_PushCommand_MaterialSetStorageBuffer(material, location, ck_arr);
}

CK_DLL_MFUN(material_set_sampler)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);
    SG_Sampler sampler    = SG_Sampler::fromCkObj(GET_NEXT_OBJECT(ARGS));

    SG_Material::setSampler(material, location, sampler);

    CQ_PushCommand_MaterialSetSampler(material, location, sampler);
}

CK_DLL_MFUN(material_set_texture)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);
    Chuck_Object* ckobj   = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex       = SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id));

    SG_Material::setTexture(material, location, tex);

    CQ_PushCommand_MaterialSetTexture(material, location, tex);
}

// Lines2DMaterial ===================================================================

CK_DLL_CTOR(lines2d_material_ctor)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    material->material_type = SG_MATERIAL_LINES2D;

    // init shader
    // Shader lines2d_shader(lines2d_shader_desc);
    SG_Shader* lines2d_shader
      = SG_GetShader(g_material_builtin_shaders.lines2d_shader_id);
    ASSERT(lines2d_shader);

    chugl_materialSetShader(material, lines2d_shader);

    // set pso
    material->pso.primitive_topology = WGPUPrimitiveTopology_TriangleStrip;
    CQ_PushCommand_MaterialUpdatePSO(material);

    // set uniform
    // TODO where to store default uniform values + locations?
    SG_Material::uniformFloat(material, 0, 0.1f); // thickness
    // TODO extrusion uniform

    CQ_PushCommand_MaterialSetUniform(material, 0);
}

CK_DLL_MFUN(lines2d_material_get_thickness)
{
    RETURN->v_float = GET_MATERIAL(SELF)->uniforms[0].as.f;
}

CK_DLL_MFUN(lines2d_material_set_thickness)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKFLOAT thickness   = GET_NEXT_FLOAT(ARGS);

    SG_Material::uniformFloat(material, 0, (f32)thickness);
    CQ_PushCommand_MaterialSetUniform(material, 0);
}

// FlatMaterial ===================================================================

CK_DLL_CTOR(flat_material_ctor)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    material->material_type = SG_MATERIAL_FLAT;

    // init shader
    // Shader lines2d_shader(lines2d_shader_desc);
    SG_Shader* lines2d_shader = SG_GetShader(g_material_builtin_shaders.flat_shader_id);
    ASSERT(lines2d_shader);

    chugl_materialSetShader(material, lines2d_shader);

    // set pso
    // material->pso.primitive_topology = WGPUPrimitiveTopology_TriangleStrip;
    // CQ_PushCommand_MaterialUpdatePSO(material);

    // set uniform
    // TODO where to store default uniform values + locations?
    SG_Material::uniformVec4f(material, 0, glm::vec4(1.0f)); // thickness

    CQ_PushCommand_MaterialSetUniform(material, 0);
}

CK_DLL_MFUN(flat_material_get_color)
{
    glm::vec4 color = GET_MATERIAL(SELF)->uniforms[0].as.vec4f;
    RETURN->v_vec4  = { color.r, color.g, color.b, color.a };
}

CK_DLL_MFUN(flat_material_set_color)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKVEC4 color        = GET_NEXT_VEC4(ARGS);

    SG_Material::uniformVec4f(material, 0,
                              glm::vec4(color.x, color.y, color.z, color.w));
    CQ_PushCommand_MaterialSetUniform(material, 0);
}

// PBRMaterial ===================================================================

CK_DLL_CTOR(pbr_material_ctor)
{
    // SG_Material* material = GET_MATERIAL(SELF);
    // ASSERT(material->type == SG_COMPONENT_MATERIAL);
    // ASSERT(material->material_type == SG_MATERIAL_PBR);

    // OBJ_MEMBER_UINT(SELF, component_offset_id) = material->id;

    // CQ_PushCommand_MaterialCreate(material);
}

// init default materials ========================================================

static SG_ID chugl_createShader(CK_DL_API API, const char* vertex_string,
                                const char* fragment_string,
                                const char* vertex_filepath,
                                const char* fragment_filepath,
                                WGPUVertexFormat* vertex_layout,
                                int vertex_layout_count)
{
    Chuck_Object* shader_ckobj
      = chugin_createCkObj(SG_CKNames[SG_COMPONENT_SHADER], true);

    // create shader on audio side
    SG_Shader* shader
      = SG_CreateShader(shader_ckobj, vertex_string, fragment_string, vertex_filepath,
                        fragment_filepath, vertex_layout, vertex_layout_count);

    // save component id
    OBJ_MEMBER_UINT(shader_ckobj, component_offset_id) = shader->id;

    // push to command queue
    CQ_PushCommand_ShaderCreate(shader);

    return shader->id;
}

void chugl_initDefaultMaterials()
{
    static WGPUVertexFormat standard_vertex_layout[] = {
        WGPUVertexFormat_Float32x3, // position
        WGPUVertexFormat_Float32x3, // normal
        WGPUVertexFormat_Float32x2, // uv
        WGPUVertexFormat_Float32x4, // tangent
    };

    static WGPUVertexFormat gtext_vertex_layout[] = {
        WGPUVertexFormat_Float32x2, // position
        WGPUVertexFormat_Float32x2, // uv
        WGPUVertexFormat_Sint32,    // glyph_index
    };

    g_material_builtin_shaders.lines2d_shader_id = chugl_createShader(
      g_chuglAPI, lines2d_shader_string, lines2d_shader_string, NULL, NULL, NULL, 0);
    g_material_builtin_shaders.flat_shader_id = chugl_createShader(
      g_chuglAPI, flat_shader_string, flat_shader_string, NULL, NULL,
      standard_vertex_layout, ARRAY_LENGTH(standard_vertex_layout));
    g_material_builtin_shaders.gtext_shader_id = chugl_createShader(
      g_chuglAPI, gtext_shader_string, gtext_shader_string, NULL, NULL,
      gtext_vertex_layout, ARRAY_LENGTH(gtext_vertex_layout));
}
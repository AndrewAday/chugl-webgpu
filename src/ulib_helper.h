#pragma once

#include <chuck/chugin.h>
#include <unordered_map>
#include <webgpu/webgpu.h>

#include "core/macros.h"
#include "core/memory.h"

// TODO: group all this shared state together into a "chugl_audio_context"
// struct, similar to App or Renderer struct

#define BEGIN_CLASS(type, base) QUERY->begin_class(QUERY, type, base)
#define END_CLASS() QUERY->end_class(QUERY)
#define CTOR(func) QUERY->add_ctor(QUERY, func)
#define DTOR(func) QUERY->add_dtor(QUERY, func)
#define MVAR(type, name, is_const) QUERY->add_mvar(QUERY, type, name, is_const)
#define MFUN(func, ret, name) QUERY->add_mfun(QUERY, func, ret, name)
#define SFUN(func, ret, name) QUERY->add_sfun(QUERY, func, ret, name)
#define SVAR(type, name, val) QUERY->add_svar(QUERY, type, name, true, val);
#define ARG(type, name) QUERY->add_arg(QUERY, type, name)
#define DOC_FUNC(doc) QUERY->doc_func(QUERY, doc)
#define DOC_CLASS(doc) QUERY->doc_class(QUERY, doc)
#define DOC_VAR(doc) QUERY->doc_var(QUERY, doc)

#define GET_NEXT_INT_ARRAY(ptr) (*((Chuck_ArrayInt**&)ptr)++)
#define GET_NEXT_FLOAT_ARRAY(ptr) (*((Chuck_ArrayFloat**&)ptr)++)
#define GET_NEXT_VEC2_ARRAY(ptr) (*((Chuck_ArrayVec2**&)ptr)++)
#define GET_NEXT_OBJECT_ARRAY(ptr) (*((Chuck_ArrayInt**&)ptr)++)

#define OBJ_MEMBER_INT_ARRAY(obj, offset)                                              \
    (*(Chuck_ArrayInt**)OBJ_MEMBER_DATA(obj, offset))
#define OBJ_MEMBER_FLOAT_ARRAY(obj, offset)                                            \
    (*(Chuck_ArrayFloat**)OBJ_MEMBER_DATA(obj, offset))

// log levels (copied from
// https://github.com/ccrma/chuck/blob/90f966cb8649840bc05f5d77219867593eb7fe94/src/core/chuck_errmsg.h#L78)
#define CK_LOG_ALL 10 // set this to log everything
#define CK_LOG_FINEST 9
#define CK_LOG_FINER 8
#define CK_LOG_FINE 7
#define CK_LOG_DEBUG 6 // 1.5.0.5 was: CK_LOG_CONFIG
#define CK_LOG_INFO 5
#define CK_LOG_WARNING 4
#define CK_LOG_HERALD 3
#define CK_LOG_SYSTEM 2
#define CK_LOG_CORE 1
#define CK_LOG_NONE 0 // set this to log nothing

#define CK_LOG(level, text) g_chuglAPI->vm->em_log(level, text)
#define CK_THROW(exception, desc, shred)                                               \
    g_chuglAPI->vm->throw_exception(exception, desc, shred)

#define CHUGIN_SAFE_DELETE(type, data_offset)                                          \
    type* obj = (type*)OBJ_MEMBER_UINT(SELF, data_offset);                             \
    if (obj) delete obj;                                                               \
    OBJ_MEMBER_UINT(SELF, data_offset) = 0;

// references to VM and API
Chuck_VM* g_chuglVM  = NULL;
CK_DL_API g_chuglAPI = NULL;

// offset which stores the component's SG_ID.
static t_CKUINT component_offset_id = 0;

// vtable offsets
static t_CKINT ggen_update_vt_offset = -1;

// SG_Sampler offsets
// idea: store these as static ints on SG_Sampler struct?
static t_CKUINT sampler_offset_wrapU     = 0;
static t_CKUINT sampler_offset_wrapV     = 0;
static t_CKUINT sampler_offset_wrapW     = 0;
static t_CKUINT sampler_offset_filterMin = 0;
static t_CKUINT sampler_offset_filterMag = 0;
static t_CKUINT sampler_offset_filterMip = 0;

// other shared data offset
// static t_CKUINT b2_world_data_offset = 0;

struct chugl_MaterialBuiltinShaders {
    SG_ID lines2d_shader_id;
    SG_ID flat_shader_id;
    SG_ID gtext_shader_id;

    // screen shaders
    SG_ID output_pass_shader_id;

    // compute shaders
    SG_ID bloom_downsample_shader_id;
    SG_ID bloom_upsample_shader_id;
};
static chugl_MaterialBuiltinShaders g_material_builtin_shaders;

struct chugl_builtin_textures {
    SG_ID white_pixel_id;
    SG_ID black_pixel_id;
    SG_ID default_render_texture_id;
};
static chugl_builtin_textures g_builtin_textures;

// impl in ulib_texture.cpp
SG_Texture* ulib_texture_createTexture(SG_TextureDesc desc);

Arena audio_frame_arena;

// map from ckobj to shred
std::unordered_map<Chuck_Object*, Chuck_VM_Shred*> ckobj_2_OriginShred;

void chugin_setVTableOffset(t_CKINT* offset, const char* type_name,
                            const char* method_name)
{
    // update() vt offset
    Chuck_Type* cktype = g_chuglAPI->type->lookup(g_chuglVM, type_name);
    // find the offset for update
    *offset = g_chuglAPI->type->get_vtable_offset(g_chuglVM, cktype, method_name);
}

// store ckobj --> ancestor shred mapping
void chugin_setOriginShred(Chuck_Object* ckobj, Chuck_VM_Shred* shred)
{
    Chuck_VM_Shred* parent_shred = shred;
    // walk up parent chain until top-level
    while (g_chuglAPI->shred->parent(parent_shred)) {
        parent_shred = g_chuglAPI->shred->parent(parent_shred);
    }

    ckobj_2_OriginShred[ckobj] = parent_shred;
}

Chuck_VM_Shred* chugin_getOriginShred(Chuck_Object* ckobj)
{
    return ckobj_2_OriginShred.find(ckobj) == ckobj_2_OriginShred.end() ?
             NULL :
             ckobj_2_OriginShred[ckobj];
}

Chuck_VM_Shred* chugin_removeFromOriginShredMap(Chuck_Object* ckobj)
{
    Chuck_VM_Shred* shred = chugin_getOriginShred(ckobj);
    ckobj_2_OriginShred.erase(ckobj);
    return shred;
}

Chuck_Object* chugin_createCkObj(const char* type_name, bool add_ref,
                                 Chuck_VM_Shred* shred)
{
    Chuck_DL_Api::Type cktype = g_chuglAPI->type->lookup(g_chuglVM, type_name);
    return g_chuglAPI->object->create(shred, cktype, add_ref);
}

Chuck_Object* chugin_createCkObj(const char* type_name, bool add_ref)
{
    Chuck_DL_Api::Type cktype = g_chuglAPI->type->lookup(g_chuglVM, type_name);
    return g_chuglAPI->object->create_without_shred(g_chuglVM, cktype, add_ref);
}

const char* chugin_copyCkString(Chuck_String* ck_str)
{
    return strdup(g_chuglAPI->object->str(ck_str));
}

Chuck_String* chugin_createCkString(const char* str)
{
    return g_chuglAPI->object->create_string(g_chuglVM, str, false);
}

// copies up to count elements from ck_arr to arr
void chugin_copyCkIntArray(Chuck_ArrayInt* ck_arr, int* arr, int count)
{
    int size = MIN(g_chuglAPI->object->array_int_size(ck_arr), count);
    for (int i = 0; i < size; i++) {
        arr[i] = (int)g_chuglAPI->object->array_int_get_idx(ck_arr, i);
    }
}

// copies up to count elements from ck_arr to arr
void chugin_copyCkFloatArray(Chuck_ArrayFloat* ck_arr, float* arr, int count)
{
    int size = MIN(g_chuglAPI->object->array_float_size(ck_arr), count);
    for (int i = 0; i < size; i++) {
        arr[i] = (f32)g_chuglAPI->object->array_float_get_idx(ck_arr, i);
    }
}

void chugin_copyCkVec2Array(Chuck_ArrayVec2* ck_arr, f32* arr)
{
    int size = g_chuglAPI->object->array_vec2_size(ck_arr);
    for (int i = 0; i < size; i++) {
        t_CKVEC2 vec2  = g_chuglAPI->object->array_vec2_get_idx(ck_arr, i);
        arr[i * 2]     = vec2.x;
        arr[i * 2 + 1] = vec2.y;
    }
}

void chugin_copyCkVec3Array(Chuck_ArrayVec3* ck_arr, f32* arr)
{
    int size = g_chuglAPI->object->array_vec3_size(ck_arr);
    for (int i = 0; i < size; i++) {
        t_CKVEC3 vec3  = g_chuglAPI->object->array_vec3_get_idx(ck_arr, i);
        arr[i * 3]     = vec3.x;
        arr[i * 3 + 1] = vec3.y;
        arr[i * 3 + 2] = vec3.z;
    }
}

void chugin_copyCkVec4Array(Chuck_ArrayVec4* ck_arr, f32* arr)
{
    int size = g_chuglAPI->object->array_vec4_size(ck_arr);
    for (int i = 0; i < size; i++) {
        t_CKVEC4 vec4  = g_chuglAPI->object->array_vec4_get_idx(ck_arr, i);
        arr[i * 4]     = vec4.x;
        arr[i * 4 + 1] = vec4.y;
        arr[i * 4 + 2] = vec4.z;
        arr[i * 4 + 3] = vec4.w;
    }
}

Chuck_ArrayInt* chugin_createCkIntArray(int* arr, int count)
{
    Chuck_ArrayInt* ck_arr = (Chuck_ArrayInt*)chugin_createCkObj("int[]", false);
    ASSERT(g_chuglAPI->object->array_int_size(ck_arr) == 0);
    for (int i = 0; i < count; i++) {
        g_chuglAPI->object->array_int_push_back(ck_arr, arr[i]);
    }
    return ck_arr;
}

Chuck_ArrayFloat* chugin_createCkFloatArray(float* arr, int count)
{
    Chuck_ArrayFloat* ck_arr = (Chuck_ArrayFloat*)chugin_createCkObj("float[]", false);
    ASSERT(g_chuglAPI->object->array_float_size(ck_arr) == 0);
    for (int i = 0; i < count; i++) {
        g_chuglAPI->object->array_float_push_back(ck_arr, arr[i]);
    }
    return ck_arr;
}

bool chugin_typeEquals(Chuck_Object* ckobj, const char* type_name)
{
    Chuck_DL_Api::Type ggenType = g_chuglAPI->type->lookup(g_chuglVM, type_name);
    Chuck_DL_Api::Type thisType = g_chuglAPI->object->get_type(ckobj);
    return (
      g_chuglAPI->type->is_equal(
        thisType,
        ggenType) // this type is an exact match (subclasses are handled on their own)
      || g_chuglAPI->type->origin_hint(thisType)
           == ckte_origin_USERDEFINED // this type is defined .ck file
      || g_chuglAPI->type->origin_hint(thisType)
           == ckte_origin_IMPORT // .ck file included in search path
    );
}

// impl in ulib_material.cpp
void chugl_materialSetShader(SG_Material* material, SG_Shader* shader);
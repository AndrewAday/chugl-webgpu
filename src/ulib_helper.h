#pragma once

#include <chuck/chugin.h>
#include <unordered_map>

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
#define GET_NEXT_OBJECT_ARRAY(ptr) (*((Chuck_ArrayInt**&)ptr)++)

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
#define CK_THROW(exception, desc, shred)                                       \
    g_chuglAPI->vm->throw_exception(exception, desc, shred)

#define CHUGIN_SAFE_DELETE(type, data_offset)                                  \
    type* obj = (type*)OBJ_MEMBER_UINT(SELF, data_offset);                     \
    if (obj) delete obj;                                                       \
    OBJ_MEMBER_UINT(SELF, data_offset) = 0;

// references to VM and API
Chuck_VM* g_chuglVM  = NULL;
CK_DL_API g_chuglAPI = NULL;

// offset which stores the component's SG_ID.
static t_CKUINT component_offset_id = 0;

// vtable offsets
static t_CKINT ggen_update_vt_offset = -1;

// other shared data offset
// static t_CKUINT b2_world_data_offset = 0;

Arena audio_frame_arena;

// map from ckobj to shred
std::unordered_map<Chuck_Object*, Chuck_VM_Shred*> ckobj_2_OriginShred;

void chugin_setVTableOffset(t_CKINT* offset, const char* type_name,
                            const char* method_name)
{
    // update() vt offset
    Chuck_Type* cktype = g_chuglAPI->type->lookup(g_chuglVM, type_name);
    // find the offset for update
    *offset
      = g_chuglAPI->type->get_vtable_offset(g_chuglVM, cktype, method_name);
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

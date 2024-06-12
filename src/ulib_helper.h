#pragma once

#include <chuck/chugin.h>
#include <unordered_map>

#include "core/macros.h"

// TODO: group all this shared state together into a "chugl_audio_context"
// struct, similar to App or Renderer struct

// references to VM and API
Chuck_VM* g_chuglVM  = NULL;
CK_DL_API g_chuglAPI = NULL;

// vtable offsets
static t_CKINT ggen_update_vt_offset = -1;

// metadata required for scene rendering
struct GG_Config {
    SG_ID mainScene;
    SG_ID mainCamera;
};

GG_Config gg_config = {};

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

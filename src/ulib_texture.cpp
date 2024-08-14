#include <chuck/chugin.h>

#include "sg_component.h"

#include "ulib_helper.h"

#define GET_TEXTURE(ckobj) SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id))

CK_DLL_CTOR(sampler_ctor);

CK_DLL_CTOR(texture_ctor);
CK_DLL_MFUN(texture_data);

static void ulib_texture_query(Chuck_DL_Query* QUERY)
{
    { // Sampler (only passed by value)
        QUERY->begin_class(QUERY, "TextureSampler", "Object");

        // static vars
        static t_CKINT WRAP_REPEAT    = SG_SAMPLER_WRAP_REPEAT;
        static t_CKINT WRAP_MIRROR    = SG_SAMPLER_WRAP_MIRROR_REPEAT;
        static t_CKINT WRAP_CLAMP     = SG_SAMPLER_WRAP_CLAMP_TO_EDGE;
        static t_CKINT FILTER_NEAREST = SG_SAMPLER_FILTER_NEAREST;
        static t_CKINT FILTER_LINEAR  = SG_SAMPLER_FILTER_LINEAR;
        QUERY->add_svar(QUERY, "int", "WRAP_REPEAT", true, &WRAP_REPEAT);
        QUERY->add_svar(QUERY, "int", "WRAP_MIRROR", true, &WRAP_MIRROR);
        QUERY->add_svar(QUERY, "int", "WRAP_CLAMP", true, &WRAP_CLAMP);
        QUERY->add_svar(QUERY, "int", "FILTER_NEAREST", true, &FILTER_NEAREST);
        QUERY->add_svar(QUERY, "int", "FILTER_LINEAR", true, &FILTER_LINEAR);

        // member vars
        sampler_offset_wrapU     = QUERY->add_mvar(QUERY, "int", "wrapU", false);
        sampler_offset_wrapV     = QUERY->add_mvar(QUERY, "int", "wrapV", false);
        sampler_offset_wrapW     = QUERY->add_mvar(QUERY, "int", "wrapW", false);
        sampler_offset_filterMin = QUERY->add_mvar(QUERY, "int", "filterMin", false);
        sampler_offset_filterMag = QUERY->add_mvar(QUERY, "int", "filterMag", false);
        sampler_offset_filterMip = QUERY->add_mvar(QUERY, "int", "filterMip", false);

        // constructor
        QUERY->add_ctor(QUERY, sampler_ctor); // default constructor

        QUERY->end_class(QUERY); // Sampler
    }

    // Texture
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_TEXTURE], SG_CKNames[SG_COMPONENT_BASE]);

    CTOR(texture_ctor);

    MFUN(texture_data, "void", "data");
    ARG("int[]", "pixel_data");
    ARG("int", "width");  // in pixels
    ARG("int", "height"); // in pixels

    // TODO: specify in WGPUImageCopyTexture where in texture to write to ?
    // e.g. texture.subData()

    END_CLASS();
}

CK_DLL_CTOR(sampler_ctor)
{
    // default to repeat wrapping and linear filtering
    OBJ_MEMBER_INT(SELF, sampler_offset_wrapU)     = SG_SAMPLER_WRAP_REPEAT;
    OBJ_MEMBER_INT(SELF, sampler_offset_wrapV)     = SG_SAMPLER_WRAP_REPEAT;
    OBJ_MEMBER_INT(SELF, sampler_offset_wrapW)     = SG_SAMPLER_WRAP_REPEAT;
    OBJ_MEMBER_INT(SELF, sampler_offset_filterMin) = SG_SAMPLER_FILTER_LINEAR;
    OBJ_MEMBER_INT(SELF, sampler_offset_filterMag) = SG_SAMPLER_FILTER_LINEAR;
    OBJ_MEMBER_INT(SELF, sampler_offset_filterMip) = SG_SAMPLER_FILTER_LINEAR;
}

// Texture =====================================================================

CK_DLL_CTOR(texture_ctor)
{
    SG_Texture* tex                            = SG_CreateTexture(SELF);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = tex->id;
    CQ_PushCommand_TextureCreate(tex);
}

CK_DLL_MFUN(texture_data)
{
    SG_Texture* tex = GET_TEXTURE(SELF);

    // TODO move into member fn?
    {
        Chuck_ArrayInt* ck_arr = GET_NEXT_INT_ARRAY(ARGS);
        t_CKUINT width         = GET_NEXT_INT(ARGS);
        t_CKUINT height        = GET_NEXT_INT(ARGS);

        int ck_arr_len = API->object->array_int_size(ck_arr);

        // check if pixel data is correct size
        ASSERT(ck_arr_len == width * height * 4); // RGBA

        // reset texture pixel data (assume we're always re-writing entire thing)
        Arena::clear(&tex->data);
        tex->width  = width;
        tex->height = height;

        // copy pixel data from ck array
        u8* pixels = ARENA_PUSH_COUNT(&tex->data, u8, ck_arr_len);
        for (int i = 0; i < ck_arr_len; i++) {
            pixels[i] = (u8)API->object->array_int_get_idx(ck_arr, i);
        }

        ASSERT(tex->data.curr == width * height * 4); // write correct # of bytes
    }

    // TODO push cq
    CQ_PushCommand_TextureData(tex);
}
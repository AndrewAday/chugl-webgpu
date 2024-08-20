#include <chuck/chugin.h>

#include "sg_command.h"
#include "sg_component.h"

#include "ulib_helper.h"

#define GET_TEXTURE(ckobj) SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id))
void ulib_texture_createDefaults(CK_DL_API API);

CK_DLL_CTOR(sampler_ctor);

CK_DLL_CTOR(texture_ctor);
CK_DLL_CTOR(texture_ctor_with_desc);

CK_DLL_MFUN(texture_data);
CK_DLL_MFUN(texture_set_file);

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

    CTOR(texture_ctor_with_desc);
    ARG("int", "is_storage");

    MFUN(texture_data, "void", "data");
    ARG("int[]", "pixel_data");
    ARG("int", "width");  // in pixels
    ARG("int", "height"); // in pixels

    MFUN(texture_set_file, "void", "file");
    ARG("string", "filepath");

    // TODO: specify in WGPUImageCopyTexture where in texture to write to ?
    // e.g. texture.subData()

    END_CLASS();

    ulib_texture_createDefaults(QUERY->ck_api(QUERY));
}

// create default pixel textures and samplers
void ulib_texture_createDefaults(CK_DL_API API)
{
    Chuck_Object* white_pixel_ckobj
      = chugin_createCkObj(SG_CKNames[SG_COMPONENT_TEXTURE], true);
    { // creating via chugin API doesn't call ctor
        SG_Texture* tex = SG_CreateTexture(white_pixel_ckobj);
        OBJ_MEMBER_UINT(white_pixel_ckobj, component_offset_id) = tex->id;
        CQ_PushCommand_TextureCreate(tex, false);

        // upload pixel data
        u8 white_pixel[] = { 255, 255, 255, 255 };
        SG_Texture::write(tex, white_pixel, 1, 1);
        CQ_PushCommand_TextureData(tex);

        // set global
        g_builtin_textures.white_pixel_id = tex->id;
    }
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
    CQ_PushCommand_TextureCreate(tex, false);
}

CK_DLL_CTOR(texture_ctor_with_desc)
{
    SG_Texture* tex                            = SG_CreateTexture(SELF);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = tex->id;
    bool is_storage                            = GET_NEXT_INT(ARGS);

    // TODO store texture desc state (like material pso)

    CQ_PushCommand_TextureCreate(tex, is_storage);
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

CK_DLL_MFUN(texture_set_file)
{
    SG_Texture* tex      = GET_TEXTURE(SELF);
    Chuck_String* ck_str = GET_NEXT_STRING(ARGS);

    // For now NOT doing stb file IO on audio thread.
    // 1) don't have to do file IO
    // 2) don't have to pass texture data over command queue

    CQ_PushCommand_TextureFromFile(tex, API->object->str(ck_str));
}
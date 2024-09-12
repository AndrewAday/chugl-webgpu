#include <chuck/chugin.h>

#include "sg_command.h"
#include "sg_component.h"

#include "ulib_helper.h"

#if 0
enum WGPUTextureUsage {
    WGPUTextureUsage_CopySrc = 0x00000001,
    WGPUTextureUsage_CopyDst = 0x00000002,
    WGPUTextureUsage_TextureBinding = 0x00000004,
    WGPUTextureUsage_StorageBinding = 0x00000008,
    WGPUTextureUsage_RenderAttachment = 0x00000010,
}

enum WGPUTextureDimension {
    WGPUTextureDimension_1D = 0x00000000,
    WGPUTextureDimension_2D = 0x00000001,
    WGPUTextureDimension_3D = 0x00000002,
}

struct WGPUExtent3D {
    uint32_t width;
    uint32_t height;
    uint32_t depthOrArrayLayers;
}

struct WGPUTextureFormat {
    WGPUTextureFormat_RGBA8Unorm = 0x00000012,
    WGPUTextureFormat_RGBA16Float = 0x00000021,
    WGPUTextureFormat_Depth24PlusStencil8 = 0x00000028,
}

struct WGPUTextureDescriptor {
    WGPUTextureUsageFlags usage;    // 
    WGPUTextureDimension dimension; //  
    WGPUExtent3D size;
    WGPUTextureFormat format;
    // uint32_t mipLevelCount; // always gen mips
    // uint32_t sampleCount;   // don't expose for now
}

typedef struct WGPUOrigin3D {
    uint32_t x;
    uint32_t y;
    uint32_t z;
}

enum WGPUTextureAspect {
    WGPUTextureAspect_All = 0x00000000,
    WGPUTextureAspect_StencilOnly = 0x00000001,
    WGPUTextureAspect_DepthOnly = 0x00000002,
    WGPUTextureAspect_Force32 = 0x7FFFFFFF
}

struct WGPUImageCopyTexture {
    WGPUTexture texture;
    uint32_t mipLevel;
    WGPUOrigin3D origin;
    WGPUTextureAspect aspect; // default to Aspect_All for non-depth textures
}

struct WGPUTextureDataLayout {
    uint64_t offset; // offset into CPU-pointer void* data
    uint32_t bytesPerRow; // minimum value for bytesPerRow that is set to 256 by the API.
    uint32_t rowsPerImage; // required if there are multiple images (3D or 2D array textures)
}

void wgpuQueueWriteTexture(
    WGPUQueue queue, 
    WGPUImageCopyTexture* destination, 
    void* data, 
    size_t dataSize, 
    WGPUTextureDataLayout* dataLayout,  // layout of cpu-side void* data
    WGPUExtent3D* writeSize             // size of destination region in texture
);

#endif

#define GET_TEXTURE(ckobj) SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id))
void ulib_texture_createDefaults(CK_DL_API API);

CK_DLL_CTOR(sampler_ctor);

CK_DLL_CTOR(texture_ctor);
CK_DLL_CTOR(texture_ctor_usage_dimension_format);

CK_DLL_MFUN(texture_write);
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
    {
        BEGIN_CLASS(SG_CKNames[SG_COMPONENT_TEXTURE], SG_CKNames[SG_COMPONENT_BASE]);

        // svars ---------------
        static t_CKINT texture_usage_copy_src        = WGPUTextureUsage_CopySrc;
        static t_CKINT texture_usage_copy_dst        = WGPUTextureUsage_CopyDst;
        static t_CKINT texture_usage_texture_binding = WGPUTextureUsage_TextureBinding;
        static t_CKINT texture_usage_storage_binding = WGPUTextureUsage_StorageBinding;
        static t_CKINT texture_usage_render_attachment
          = WGPUTextureUsage_RenderAttachment;
        SVAR("int", "Usage_CopySrc", &texture_usage_copy_src);
        SVAR("int", "Usage_CopyDst", &texture_usage_copy_dst);
        SVAR("int", "Usage_TextureBinding", &texture_usage_texture_binding);
        SVAR("int", "Usage_StorageBinding", &texture_usage_storage_binding);
        SVAR("int", "Usage_RenderAttachment", &texture_usage_render_attachment);

        static t_CKINT texture_dimension_1d = WGPUTextureDimension_1D;
        static t_CKINT texture_dimension_2d = WGPUTextureDimension_2D;
        static t_CKINT texture_dimension_3d = WGPUTextureDimension_3D;
        SVAR("int", "Dimension_1D", &texture_dimension_1d);
        SVAR("int", "Dimension_2D", &texture_dimension_2d);
        SVAR("int", "Dimension_3D", &texture_dimension_3d);

        static t_CKINT texture_format_rgba8unorm  = WGPUTextureFormat_RGBA8Unorm;
        static t_CKINT texture_format_rgba16float = WGPUTextureFormat_RGBA16Float;
        // static t_CKINT texture_format_depth24plusstencil8
        //   = WGPUTextureFormat_Depth24PlusStencil8;
        SVAR("int", "Format_RGBA8Unorm", &texture_format_rgba8unorm);
        SVAR("int", "Format_RGBA16Float", &texture_format_rgba16float);
        // SVAR("int", "Format_Depth24PlusStencil8",
        // &texture_format_depth24plusstencil8);

        // mfun ---------------

        CTOR(texture_ctor);

        CTOR(texture_ctor_usage_dimension_format);
        ARG("int", "usage_flags");
        ARG("int", "dimension");
        ARG("int", "format");

        MFUN(texture_write, "void", "write");
        ARG("int[]", "pixel_data"); // assumed rgba, so 4 ints per pixel
        ARG("int", "width");        // in pixels
        ARG("int", "height");       // in pixels

        // MFUN(texture_write_base, "void", "write");
        // ARG("int", "dst_mip_level");
        // ARG("int", "dst_origin_x");
        // ARG("int", "dst_origin_y");
        // ARG("int", "dst_origin_z");

        MFUN(texture_set_file, "void", "file");
        ARG("string", "filepath");

        // TODO: specify in WGPUImageCopyTexture where in texture to write to ?
        // e.g. texture.subData()

        END_CLASS();
    }

    ulib_texture_createDefaults(QUERY->ck_api(QUERY));
}

SG_Texture* ulib_texture_createTexture(SG_TextureDesc desc)
{
    CK_DL_API API = g_chuglAPI;

    Chuck_Object* texture_obj
      = chugin_createCkObj(SG_CKNames[SG_COMPONENT_TEXTURE], true);

    SG_Texture* tex                                   = SG_CreateTexture(texture_obj);
    OBJ_MEMBER_UINT(texture_obj, component_offset_id) = tex->id;
    tex->desc                                         = desc;
    CQ_PushCommand_TextureCreate(tex);

    return tex;
}

// create default pixel textures and samplers
void ulib_texture_createDefaults(CK_DL_API API)
{
    // white pixel
    {
        SG_Texture* tex = ulib_texture_createTexture({ WGPUTextureUsage_TextureBinding,
                                                       WGPUTextureDimension_2D,
                                                       WGPUTextureFormat_RGBA8Unorm });
        // upload pixel data
        u8 white_pixel[] = { 255, 255, 255, 255 };
        SG_Texture::write(tex, white_pixel, 1, 1);
        CQ_PushCommand_TextureData(tex);

        // set global
        g_builtin_textures.white_pixel_id = tex->id;
    }
    //  default render texture (hdr)
    {
        SG_Texture* tex = ulib_texture_createTexture(
          { WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding
              | WGPUTextureUsage_StorageBinding,
            WGPUTextureDimension_2D, WGPUTextureFormat_RGBA16Float });
        // set global
        g_builtin_textures.default_render_texture_id = tex->id;
    }

    { // black pixel
        SG_Texture* tex = ulib_texture_createTexture({ WGPUTextureUsage_TextureBinding,
                                                       WGPUTextureDimension_2D,
                                                       WGPUTextureFormat_RGBA8Unorm });
        // upload pixel data
        u8 black_pixel[] = { 255, 255, 255, 255 };
        SG_Texture::write(tex, black_pixel, 1, 1);
        CQ_PushCommand_TextureData(tex);

        // set global
        g_builtin_textures.black_pixel_id = tex->id;
    }

    { // default normal map
        SG_Texture* tex = ulib_texture_createTexture({ WGPUTextureUsage_TextureBinding,
                                                       WGPUTextureDimension_2D,
                                                       WGPUTextureFormat_RGBA8Unorm });
        // upload pixel data
        u8 normal_pixel[] = { 128, 128, 255, 255 };
        SG_Texture::write(tex, normal_pixel, 1, 1);
        CQ_PushCommand_TextureData(tex);

        // set global
        g_builtin_textures.normal_pixel_id = tex->id;
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
    CQ_PushCommand_TextureCreate(tex);
}

CK_DLL_CTOR(texture_ctor_usage_dimension_format)
{
    SG_Texture* tex                            = SG_CreateTexture(SELF);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = tex->id;
    u32 usage_flags                            = GET_NEXT_UINT(ARGS);
    WGPUTextureDimension dimension = (WGPUTextureDimension)GET_NEXT_INT(ARGS);
    WGPUTextureFormat format       = (WGPUTextureFormat)GET_NEXT_INT(ARGS);

    // store texture desc state (like material pso)
    tex->desc = { usage_flags, dimension, format };

    CQ_PushCommand_TextureCreate(tex);
}

CK_DLL_MFUN(texture_write)
{
    SG_Texture* tex = GET_TEXTURE(SELF);

    // TODO move into member fn?
    {
        Chuck_ArrayInt* ck_arr = GET_NEXT_INT_ARRAY(ARGS);
        t_CKINT width          = GET_NEXT_INT(ARGS);
        t_CKINT height         = GET_NEXT_INT(ARGS);

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
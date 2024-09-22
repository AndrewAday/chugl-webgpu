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

// ============ Impl Texture Writes =============
/*
Invariants
- Texture.size() will always create max mips
- TextureFormat and TextureDim are immutable
*/

TextureUsage_All = TextureUsage_CopySrc | TextureUsage_CopyDst | TextureUsage_TextureBinding | TextureUsage_StorageBinding | TextureUsage_RenderAttachment

class TextureDesc {
    TextureFormat format = RGBA8Unorm;
    TextureDimension dimension = 2D;

    int width = 1;
    int height = 1;
    int depth = 1;

    TextureUsageFlags usage = TextureUsage_All;

    int samples = 1;
    int mips = 0; // <= 0 means auto generate based on size
}

class TextureWriteParams {
    // Image Location
    int mip;
    int offset_x;
    int offset_y;
    int offset_z;

    // write region size
    int width;
    int height;
    int depth;

    // to add later
    // int offset
};

class TextureLoadParams {
    bool flip_vertically = false;
    bool gen_mips = true;
}

// Remember to call Queue.submit() after wgpuTextureWrite !

// TODO: do we texture load on audio thread of graphics thread?

// validation checks
// - based on texture format, determine the # of chuck array entries per texel
// - check that the length of the chuck array is >= the expected region size (width * height * depth)

#endif

#define GET_TEXTURE(ckobj) SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id))
void ulib_texture_createDefaults(CK_DL_API API);

// TextureSampler ---------------------------------------------------------------------
CK_DLL_CTOR(sampler_ctor);

// TextureDesc -----------------------------------------------------------------

static t_CKUINT texture_desc_format_offset    = 0;
static t_CKUINT texture_desc_dimension_offset = 0;
static t_CKUINT texture_desc_width_offset     = 0;
static t_CKUINT texture_desc_height_offset    = 0;
static t_CKUINT texture_desc_depth_offset     = 0;
static t_CKUINT texture_desc_usage_offset     = 0;
// static t_CKUINT texture_desc_samples_offset   = 0; // not exposing for now
static t_CKUINT texture_desc_mips_offset = 0; // not exposing for now
CK_DLL_CTOR(texture_desc_ctor);

// Texture ---------------------------------------------------------------------
CK_DLL_CTOR(texture_ctor);
CK_DLL_CTOR(texture_ctor_with_desc);

CK_DLL_MFUN(texture_get_format);
CK_DLL_MFUN(texture_get_dimension);
CK_DLL_MFUN(texture_get_width);
CK_DLL_MFUN(texture_get_height);
CK_DLL_MFUN(texture_get_depth);
CK_DLL_MFUN(texture_get_usage);
CK_DLL_MFUN(texture_get_mips);

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

    { // TextureDesc
        BEGIN_CLASS("TextureDesc", "Object");

        CTOR(texture_desc_ctor);

        // member vars
        texture_desc_format_offset    = MVAR("int", "format", false);
        texture_desc_dimension_offset = MVAR("int", "dimension", false);
        texture_desc_width_offset     = MVAR("int", "width", false);
        texture_desc_height_offset    = MVAR("int", "height", false);
        texture_desc_depth_offset     = MVAR("int", "depth", false);
        texture_desc_usage_offset     = MVAR("int", "usage", false);
        // texture_desc_samples_offset   = MVAR("int", "samples");
        texture_desc_mips_offset = MVAR("int", "mips", false);

        END_CLASS();
    } // end TextureDesc

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
        static t_CKINT texture_usage_all = WGPUTextureUsage_All;
        SVAR("int", "Usage_CopySrc", &texture_usage_copy_src);
        SVAR("int", "Usage_CopyDst", &texture_usage_copy_dst);
        SVAR("int", "Usage_TextureBinding", &texture_usage_texture_binding);
        SVAR("int", "Usage_StorageBinding", &texture_usage_storage_binding);
        SVAR("int", "Usage_RenderAttachment", &texture_usage_render_attachment);
        SVAR("int", "Usage_All", &texture_usage_all);

        static t_CKINT texture_dimension_1d = WGPUTextureDimension_1D;
        static t_CKINT texture_dimension_2d = WGPUTextureDimension_2D;
        static t_CKINT texture_dimension_3d = WGPUTextureDimension_3D;
        SVAR("int", "Dimension_1D", &texture_dimension_1d);
        SVAR("int", "Dimension_2D", &texture_dimension_2d);
        SVAR("int", "Dimension_3D", &texture_dimension_3d);

        static t_CKINT texture_format_rgba8unorm  = WGPUTextureFormat_RGBA8Unorm;
        static t_CKINT texture_format_rgba16float = WGPUTextureFormat_RGBA16Float;
        static t_CKINT texture_format_rgba32float = WGPUTextureFormat_RGBA32Float;
        static t_CKINT texture_format_r32float    = WGPUTextureFormat_R32Float;
        // static t_CKINT texture_format_depth24plusstencil8
        //   = WGPUTextureFormat_Depth24PlusStencil8;
        SVAR("int", "Format_RGBA8Unorm", &texture_format_rgba8unorm);
        SVAR("int", "Format_RGBA16Float", &texture_format_rgba16float);
        SVAR("int", "Format_RGBA32Float", &texture_format_rgba32float);
        SVAR("int", "Format_R32Float", &texture_format_r32float);
        // SVAR("int", "Format_Depth24PlusStencil8",
        // &texture_format_depth24plusstencil8);

        // mfun ---------------

        CTOR(texture_ctor);

        CTOR(texture_ctor_with_desc);
        ARG("TextureDesc", "texture_desc");

        MFUN(texture_write, "void", "write");
        ARG("int[]", "pixel_data"); // assumed rgba, so 4 ints per pixel
        ARG("int", "width");        // in pixels
        ARG("int", "height");       // in pixels

        MFUN(texture_get_format, "int", "format");
        DOC_FUNC(
          "Get the texture format (immutable). Returns a value from the "
          "Texture.Format_XXXXX enum, e.g. Texture.Format_RGBA8Unorm");

        MFUN(texture_get_dimension, "int", "dimension");
        DOC_FUNC(
          "Get the texture dimension (immutable). Returns a value from the "
          "Texture.Dimension_XXXXX enum, e.g. Texture.Dimension_2D");

        MFUN(texture_get_width, "int", "width");
        DOC_FUNC("Get the texture width (immutable)");

        MFUN(texture_get_height, "int", "height");
        DOC_FUNC("Get the texture height (immutable)");

        MFUN(texture_get_depth, "int", "depth");
        DOC_FUNC(
          "Get the texture depth (immutable). For a 2D texture, depth corresponds to "
          "the number of array layers (e.g. depth=6 for a cubemap)");

        MFUN(texture_get_usage, "int", "usage");
        DOC_FUNC(
          "Get the texture usage flags (immutable). Returns a bitmask of usage flgas "
          "from the Texture.Usage_XXXXX enum e.g. Texture.Usage_TextureBinding | "
          "Texture.Usage_RenderAttachment. By default, textures are created with ALL "
          "usages enabled");

        MFUN(texture_get_mips, "int", "mips");
        DOC_FUNC(
          "Get the number of mip levels (immutable). Returns the number of mip levels "
          "in the texture.");

        MFUN(texture_set_file, "void", "load");
        ARG("string", "filepath");

        // TODO: specify in WGPUImageCopyTexture where in texture to write to ?
        // e.g. texture.subData()

        END_CLASS();
    }

    ulib_texture_createDefaults(QUERY->ck_api(QUERY));
}

// TextureSampler ------------------------------------------------------------------

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

// TextureDesc ---------------------------------------------------------------------

CK_DLL_CTOR(texture_desc_ctor)
{
    OBJ_MEMBER_INT(SELF, texture_desc_format_offset)    = WGPUTextureFormat_RGBA8Unorm;
    OBJ_MEMBER_INT(SELF, texture_desc_dimension_offset) = WGPUTextureDimension_2D;
    OBJ_MEMBER_INT(SELF, texture_desc_width_offset)     = 1;
    OBJ_MEMBER_INT(SELF, texture_desc_height_offset)    = 1;
    OBJ_MEMBER_INT(SELF, texture_desc_depth_offset)     = 1;
    OBJ_MEMBER_INT(SELF, texture_desc_usage_offset)     = WGPUTextureUsage_All;
    // OBJ_MEMBER_INT(SELF, texture_desc_samples_offset) = 1;
    OBJ_MEMBER_INT(SELF, texture_desc_mips_offset) = 0;
}

static SG_TextureDesc ulib_texture_textureDescFromCkobj(Chuck_Object* ckobj)
{
    CK_DL_API API = g_chuglAPI;

    SG_TextureDesc desc = {};
    desc.format = (WGPUTextureFormat)OBJ_MEMBER_INT(ckobj, texture_desc_format_offset);
    desc.dimension
      = (WGPUTextureDimension)OBJ_MEMBER_INT(ckobj, texture_desc_dimension_offset);
    desc.width       = OBJ_MEMBER_INT(ckobj, texture_desc_width_offset);
    desc.height      = OBJ_MEMBER_INT(ckobj, texture_desc_height_offset);
    desc.depth       = OBJ_MEMBER_INT(ckobj, texture_desc_depth_offset);
    desc.usage_flags = OBJ_MEMBER_INT(ckobj, texture_desc_usage_offset);
    // desc.samples        = OBJ_MEMBER_INT(ckobj, texture_desc_samples_offset);
    desc.mips = OBJ_MEMBER_INT(ckobj, texture_desc_mips_offset);

    // validation happens at final layer SG_CreateTexture

    return desc;
}

// Texture -----------------------------------------------------------------

// create default pixel textures and samplers
void ulib_texture_createDefaults(CK_DL_API API)
{
    SG_TextureDesc texture_binding_desc = {};
    texture_binding_desc.usage_flags    = WGPUTextureUsage_TextureBinding;
    // white pixel
    {
        SG_Texture* tex = SG_CreateTexture(&texture_binding_desc, NULL, NULL, true);
        // upload pixel data
        u8 white_pixel[] = { 255, 255, 255, 255 };
        SG_Texture::write(tex, white_pixel, 1, 1);
        CQ_PushCommand_TextureData(tex);

        // set global
        g_builtin_textures.white_pixel_id = tex->id;
    }
    //  default render texture (hdr)
    {
        SG_TextureDesc render_texture_desc = {};
        render_texture_desc.usage_flags    = WGPUTextureUsage_RenderAttachment
                                          | WGPUTextureUsage_TextureBinding
                                          | WGPUTextureUsage_StorageBinding;
        render_texture_desc.format = WGPUTextureFormat_RGBA16Float;
        // set global
        g_builtin_textures.default_render_texture_id
          = SG_CreateTexture(&render_texture_desc, NULL, NULL, true)->id;
    }

    { // black pixel
        SG_Texture* tex = SG_CreateTexture(&texture_binding_desc, NULL, NULL, true);
        // upload pixel data
        u8 black_pixel[] = { 0, 0, 0, 0 };
        SG_Texture::write(tex, black_pixel, 1, 1);
        CQ_PushCommand_TextureData(tex);

        // set global
        g_builtin_textures.black_pixel_id = tex->id;
    }

    { // default normal map
        SG_Texture* tex = SG_CreateTexture(&texture_binding_desc, NULL, NULL, true);
        // upload pixel data
        u8 normal_pixel[] = { 128, 128, 255, 255 };
        SG_Texture::write(tex, normal_pixel, 1, 1);
        CQ_PushCommand_TextureData(tex);

        // set global
        g_builtin_textures.normal_pixel_id = tex->id;
    }
}

CK_DLL_CTOR(texture_ctor)
{
    SG_TextureDesc desc = {};
    SG_CreateTexture(&desc, SELF, SHRED, false);
}

CK_DLL_CTOR(texture_ctor_with_desc)
{
    SG_TextureDesc desc = ulib_texture_textureDescFromCkobj(GET_NEXT_OBJECT(ARGS));
    SG_CreateTexture(&desc, SELF, SHRED, false);
}

CK_DLL_MFUN(texture_get_format)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.format;
}

CK_DLL_MFUN(texture_get_dimension)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.dimension;
}

CK_DLL_MFUN(texture_get_width)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.width;
}

CK_DLL_MFUN(texture_get_height)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.height;
}

CK_DLL_MFUN(texture_get_depth)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.depth;
}

CK_DLL_MFUN(texture_get_usage)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.usage_flags;
}

CK_DLL_MFUN(texture_get_mips)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.mips;
}

CK_DLL_MFUN(texture_write)
{
    SG_Texture* tex = GET_TEXTURE(SELF);

    // TODO move into member fn?
    {
        Chuck_ArrayInt* ck_arr = GET_NEXT_INT_ARRAY(ARGS);
        t_CKUINT width         = GET_NEXT_UINT(ARGS);
        t_CKUINT height        = GET_NEXT_UINT(ARGS);

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

    CQ_PushCommand_TextureFromFile(tex, API->object->str(ck_str), true);
}

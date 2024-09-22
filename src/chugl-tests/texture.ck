// TextureDesc ==========================================
TextureDesc default_tex_desc;
T.assert(default_tex_desc.width == 1, "default texture desc width " + default_tex_desc.width);
T.assert(default_tex_desc.height == 1, "default texture desc height");
T.assert(default_tex_desc.depth == 1, "default texture desc depth");
T.assert(default_tex_desc.format == Texture.Format_RGBA8Unorm, "default texture desc format");
T.assert(default_tex_desc.usage == Texture.Usage_All, "default texture desc usage");
T.assert(default_tex_desc.mips == 0, "default texture desc mips = " + default_tex_desc.mips);

// default values =======================================
Texture default_tex;
T.assert(default_tex.width() == 1, "default texture width " + default_tex.width());
T.assert(default_tex.height() == 1, "default texture height");
T.assert(default_tex.depth() == 1, "default texture depth");
T.assert(default_tex.format() == Texture.Format_RGBA8Unorm, "default texture format");
T.assert(default_tex.usage() == Texture.Usage_All, "default texture usage");
T.assert(default_tex.mips() == 1, "default texture mips = " + default_tex.mips());

// Texture creation with desc ============================

TextureDesc tex_desc;
256 => tex_desc.width;
256 => tex_desc.height;
Texture.Format_RGBA32Float => tex_desc.format;
Texture.Usage_StorageBinding | Texture.Usage_RenderAttachment => tex_desc.usage;
0 => tex_desc.mips;

Texture tex(tex_desc);

T.assert(tex.width() == 256, "texture width " + tex.width());
T.assert(tex.height() == 256, "texture height");
T.assert(tex.depth() == 1, "texture depth");
T.assert(tex.format() == Texture.Format_RGBA32Float, "texture format");
T.assert(tex.usage() == Texture.Usage_StorageBinding | Texture.Usage_RenderAttachment, "texture usage");
T.assert(tex.mips() == 9, "texture mips");

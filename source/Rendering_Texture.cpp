#include "Rendering_Texture.h"

#include "stb/stb_image.h"


Texture::~Texture()
{
    glDeleteTextures(1, &m_handle);
}

void Texture::Bind(u32 slot)
{
    assert(slot >= GL_TEXTURE0);
    assert(slot <= GL_TEXTURE31);
    glActiveTexture(slot);
    glBindTexture(m_target, m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("Texture Bound\n");
#endif
}

enum TextureTarget : u32 {
    TextureTarget_Invalid = 0,
    TextureTarget_1D = GL_TEXTURE_1D,
    TextureTarget_2D = GL_TEXTURE_2D,
    TextureTarget_2DMultisample = GL_TEXTURE_2D_MULTISAMPLE,
    TextureTarget_3D = GL_TEXTURE_3D,
    TextureTarget_3DMultisample = GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
    TextureTarget_Count,
};

Texture::Texture(TextureParams tp)
{
    assert(tp.samples);

    if (tp.size.z) //3D
    {
        assert((!!tp.size.x) && (!!tp.size.y) && (!!tp.size.z));
        if (tp.samples == 1)
        {
            m_target = GL_TEXTURE_3D;
        }
        else
        {
            assert(false);
            m_target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
        }
    }
    else if (tp.size.y) //2D
    {
        assert((tp.size.x != 0) && (tp.size.y != 0) && (tp.size.z == 0));
        if (tp.samples == 1)
        {
            m_target = GL_TEXTURE_2D;
        }
        else
        {
            m_target = GL_TEXTURE_2D_MULTISAMPLE;
        }
    }
    else //1D
    {
        assert((tp.size.x != 0) && (tp.size.y == 0) && (tp.size.z == 0));
        if (tp.samples == 1)
        {
            m_target = GL_TEXTURE_1D;
        }
        else
            assert(false);
    }

    glGenTextures(1, &m_handle);
    Bind();

    if (tp.samples == 1)
    {
        glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, tp.minFilter);
        glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, tp.magFilter);
        glTexParameteri(m_target, GL_TEXTURE_WRAP_S, tp.wrapS);
        glTexParameteri(m_target, GL_TEXTURE_WRAP_T, tp.wrapT);
    }

    switch (m_target)
    {
    case GL_TEXTURE_3D:
    {
        glTexImage3D(m_target, 0, tp.internalFormat, tp.size.x, tp.size.y, tp.size.z, 0, tp.format, tp.type, tp.data);
        break;
    }
    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
    {
        assert(false);
        glTexImage3DMultisample(m_target, tp.samples, tp.internalFormat, tp.size.x, tp.size.y, tp.size.z, GL_TRUE);
        break;
    }
    case GL_TEXTURE_2D: 
    {
        glTexImage2D(m_target, 0, tp.internalFormat, tp.size.x, tp.size.y, 0, tp.format, tp.type, tp.data);
        break;
    }
    case GL_TEXTURE_2D_MULTISAMPLE:
    {
        glTexImage2DMultisample(m_target, tp.samples, tp.internalFormat, tp.size.x, tp.size.y, GL_TRUE);
        break;
    }
    case GL_TEXTURE_1D:
    {
        glTexImage1D(m_target, 0, tp.internalFormat, tp.size.x, 0, tp.format, tp.type, tp.data);
        break;
    }
    default:
    {
        assert(false);
        break;
    }
    }

    m_size = tp.size;
#ifdef _DEBUGPRINT
    DebugPrint("Texture Created\n");
#endif
}

Texture::Texture(const char* fileLocation, GLint colorFormat)
{
    u8* data = stbi_load(fileLocation, &m_size.x, &m_size.y, &m_bytesPerPixel, STBI_rgb_alpha);

    glGenTextures(1, &m_handle);
    Bind();
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(m_target, 0, colorFormat, m_size.x, m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
#ifdef _DEBUGPRINT
    DebugPrint("Texture Created\n");
#endif
}

Texture::Texture(u8* data, Vec2I size, GLint colorFormat)//, int32 m_bytesPerPixel)
{
    m_size = Vec3I({ size.x, size.y, 0 });
    m_bytesPerPixel = 4;

    glGenTextures(1, &m_handle);
    Bind();
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(m_target, 0, colorFormat, m_size.x, m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
#ifdef _DEBUGPRINT
    DebugPrint("Texture Created\n");
#endif
}


TextureArray::TextureArray(const char* fileLocation)
{
    u8* data = stbi_load(fileLocation, &m_size.x, &m_size.y, NULL, STBI_rgb_alpha);
    Defer{
        stbi_image_free(data);
    };
    assert(data);

    glGenTextures(1, &m_handle);
    Bind();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexImage2D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    u32 mipMapLevels = 5;
    m_spritesPerSide = { 16, 16 };
    u32 height = m_spritesPerSide.y;
    u32 width = m_spritesPerSide.x;
    u32 depth = 256;

    //SRGB8_ALPHA8 is used since the texture is encoded in sRGB color space
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipMapLevels, GL_SRGB8_ALPHA8, width, height, depth);

    //TODO: Fix
    u32 colors[16 * 16] = {};
    u32 arrayIndex = 0;
    for (u32 y = height; y--;)
    {
        for (u32 x = 0; x < width; x++)
        {
            for (u32 xp= 0; xp < 16; xp++)  //Total
            {
                for (u32 yp = 0; yp < 16; yp++)  //Total
                {
                    u32 sourceIndex = (y * 16 + yp) * 256 + x * 16 + xp;
                    u32 destIndex = yp * 16 + xp;
                    colors[destIndex] = reinterpret_cast<u32*>(data)[sourceIndex];
                }
            }
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, arrayIndex, 16, 16, 1, GL_RGBA, GL_UNSIGNED_BYTE, colors);
            arrayIndex++;
        }
    }
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
#ifdef _DEBUGPRINT
    DebugPrint("Texture Created\n");
#endif
}

void TextureArray::Update(float anisotropicAmount)
{
    if (anisotropicAmount == m_anisotropicAmount)
        return;
    m_anisotropicAmount = anisotropicAmount;
    
    Bind();
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_anisotropicAmount);
}

void TextureArray::Bind()
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("Texture Bound\n");
#endif
}


struct DDS_PIXELFORMAT
{
    u32 dwSize;
    u32 dwFlags;
    u32 dwFourCC;
    u32 dwRGBBitCount;
    u32 dwRBitMask;
    u32 dwGBitMask;
    u32 dwBBitMask;
    u32 dwABitMask;
};
static_assert(sizeof(DDS_PIXELFORMAT) == 32, "Incorrect structure size!");

typedef struct
{
    u32           dwSize;
    u32           dwFlags;
    u32           dwHeight;
    u32           dwWidth;
    u32           dwPitchOrLinearSize;
    u32           dwDepth;
    u32           dwMipMapCount;
    u32           dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    u32           dwCaps;
    u32           dwCaps2;
    u32           dwCaps3;
    u32           dwCaps4;
    u32           dwReserved2;
} DDS_HEADER;
static_assert(sizeof(DDS_HEADER) == 124, "Incorrect structure size!");

TextureCube::TextureCube(const char* fileLocation)
{
    glGenTextures(1, &m_handle);
    Bind();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    FILE* file;
    if (fopen_s(&file, fileLocation, "rb") != 0)
    {
        assert(false);
        return;
    }

    Defer{ fclose(file); };

    fseek(file, 0, SEEK_END);
    auto size = ftell(file);
    fseek(file, 0, SEEK_SET);
    u8* buffer = new u8[size];
    fread(buffer, size, 1, file);
    DDS_HEADER* header = (DDS_HEADER*)((u32*)buffer + 1);
    u8* data = (u8*)(header + 1);

    i32 levels = header->dwMipMapCount;

#if 0
    void glTexStorage3D(GL_PROXY_TEXTURE_CUBE_MAP_ARRAY,
                        levels,
                        GLenum internalformat,
                        GLsizei width,
                        GLsizei height,
                        GLsizei depth);
#endif

    GLenum targets[] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    for (int i = 0; i < arrsize(targets); ++i)
    {
        GLsizei width = header->dwWidth;
        GLsizei height = header->dwHeight;

        auto Align = [](GLsizei i) { return i + 3 & ~(3); };

        for (int level = 0; level < levels; ++level)
        {
            GLsizei bw = Align(width);
            GLsizei bh = Align(height);
            GLsizei byte_size = bw * bh;

            glCompressedTexImage2D(targets[i],
                                    level,
                                    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                                    width,
                                    height,
                                    0,
                                    byte_size,
                                    data);

            data += byte_size;
            width = Max(width >> 1, 1);
            height = Max(height >> 1, 1);
        }
    }
}

void TextureCube::Bind()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_handle);
}



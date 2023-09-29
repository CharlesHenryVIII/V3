#pragma once
#include "GL/glew.h"
#include "Math.h"
#include "Misc.h"

struct Texture {
    enum T : u32 {
        Invalid,
        Minecraft,
        Plain,
        Voxel_Indices,
        Color_Palette,
        Random,
        Count,
    };
    ENUMOPS(T);

    struct TextureParams {
        Vec3I size = { 1920, 1080, 0 }; //Always overwrite
        u32 minFilter = GL_LINEAR;
        u32 magFilter = GL_LINEAR;
        u32 wrapS = GL_REPEAT;
        u32 wrapT = GL_REPEAT;
        GLint  internalFormat = GL_RGBA;
        GLenum format = GL_RGBA;
        GLenum type = GL_UNSIGNED_BYTE;
        u32 samples = 1;

        void* data = nullptr;
    };


    Vec3I m_size = {};
    i32 m_bytesPerPixel = 0;//bytes per pixel
    GLuint m_handle = {};
    GLenum m_target = GL_TEXTURE_2D;

    Texture(Texture::TextureParams tp);
    Texture(const char* fileLocation, GLint colorFormat);
    Texture(u8* data, Vec2I size, GLint colorFormat);//, int32 m_bytesPerPixel = 2);
    ~Texture();
    void Bind(u32 slot = GL_TEXTURE0);
};

class TextureArray {
public:

    Vec2I m_size = {};
    GLuint m_handle = {};
    Vec2I m_spritesPerSide;
    float m_anisotropicAmount;


    TextureArray(const char* fileLocation);
    void Update(float anisotropicAmount);
    void Bind();
};

class TextureCube {
public:
    Vec2I m_size = {};
    GLuint m_handle = {};


    TextureCube(const char* fileLocation);
    void Bind();
};

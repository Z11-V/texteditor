#ifndef ATLAS_H
#define ATLAS_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

struct atlas
{
    unsigned int texture;
    unsigned int w; //width of texture
    unsigned int h; //height of texture
    unsigned int size;

    struct character_info
    {
        float ax; //advance x
        float ay; //advance y
        float bw; //bitmap.width
        float bh; //bitmap.rows
        float bl; //bitmap_left
        float bt; //bitmap_top
        float tx; //x offset in texture
    } c[128];

    atlas(FT_Face face, int height)
    {
        FT_Set_Pixel_Sizes(face, 0, height);
        FT_GlyphSlot g = face->glyph;
        size = height;
        w = 0;
        h = 0;

        //find texture width and height
        for (int i = 32; i < 128; i++)
        {
            if (FT_Load_Char(face, i, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            w += g->bitmap.width;
            h = std::max(h, g->bitmap.rows); 
        }

        //generate texture
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int x = 0;
        for (int i = 32; i < 128; i++)
        {
            if (FT_Load_Char(face, i, FT_LOAD_RENDER))
                continue;
            
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, g->bitmap.width, g->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);
            
            c[i].ax = g->advance.x >> 6;
            c[i].ay = g->advance.y >> 6;
            c[i].bw = g->bitmap.width;
            c[i].bh = g->bitmap.rows;
            c[i].bl = g->bitmap_left;
            c[i].bt = g->bitmap_top;
            c[i].tx = (float)x / w;

            x += g->bitmap.width;

        }
    }

    ~atlas()
    {
        glDeleteTextures(1, &texture);
    }
    
};


#endif
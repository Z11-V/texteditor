#ifndef RENDERENGINE_H
#define RENDERENGINE_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include <GLFW/glfw3.h>
#include <string_view>
#include <array>
#include <cmath>
#include "shader.h"
#include "textbuffer.h"
#include "atlas.h"


class Renderengine
{
private:
    unsigned int text_VAO,text_VBO;
    unsigned int cursor_VAO, cursor_VBO;
    Shader text_shader{"shaders/text.vert","shaders/text.frag"};
    Shader cursor_shader{"shaders/cursor.vert","shaders/cursor.frag"};
public:
    float start_pos_x = 4,start_pos_y = 0,cursor_pos_x = 0,cursor_pos_x_p = 0;
    float scale_x{}, scale_y{};
    float line_spacing{2};

    float scroll_x = 0, scroll_y = 0;
    std::array<int,2> view_bounds_y{};
    float ymax{0};
    float xmax{0};

    std::array<int,2>& cursor;
    atlas* a;
    std::vector<std::string_view>& view;
    int& view_size;

    std::array<float,3> bgcolor = {1.0,1.0,1.0};
    std::array<float,3> txtcolor = {0,0,0};
    std::array<float,3> cursorcolor = {0.45,0.45,0.45};

    Renderengine(int winw, int winh, atlas* atlas,std::array<int,2>& _cursor, std::vector<std::string_view>& _view, int& _view_size);
    ~Renderengine();
    void draw();
    void render_text();
    void render_cursor();
    void set_scroll(float yoffset, float xoffset = 0);
};

Renderengine::Renderengine(int winw, int winh, atlas* atlas,std::array<int,2>& _cursor, std::vector<std::string_view>& _view, int& _view_size): cursor(_cursor), view(_view), view_size(_view_size)
{
    glViewport(0, 0, winw, winh);
    scale_x = 2.0/winw;
    scale_y = 2.0/winh;
    a = atlas;

    //glEnable(GL_CULL_FACE); //if culling enabled need to draw vertices in order(don't)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(bgcolor[0], bgcolor[1], bgcolor[2], 1.0f);
     
    glGenVertexArrays(1, &text_VAO);
    glGenBuffers(1, &text_VBO);
    glBindVertexArray(text_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, text_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*6*4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(float),0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &cursor_VAO);
    glGenBuffers(1, &cursor_VBO);
    glBindVertexArray(cursor_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, cursor_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*6*2, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float),0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Renderengine::~Renderengine()
{
}

void Renderengine::render_text()
{
    //set view and atlas before calling
    glBindTexture(GL_TEXTURE_2D, a->texture);
    glBindVertexArray(text_VAO);
    glBindBuffer(GL_ARRAY_BUFFER,text_VBO);

    struct point
    {
        float x;
        float y;
        float s;
        float t;
    }data[6*view_size];

    int n = 0; //no of points
    int line_no = 1;
    
    float x = -1 + (start_pos_x+scroll_x)*scale_x;
    float y = 1 - (line_no*(a->h+line_spacing)*scale_y) -((start_pos_y+scroll_y)*scale_y);

    view_bounds_y[0] = -1;
    xmax = 0;
    for (auto line : view)
    {
        int ch_no = 0; //no of characters rendered in a line
        float x_p = start_pos_x+2; // x in pixels
        for (auto ch : line)
        {
            float xpos = x + a->c[ch].bl * scale_x;
            float ypos = y -(a->c[ch].bh - a->c[ch].bt) * scale_y;

            float w = a->c[ch].bw * scale_x;
            float h = a->c[ch].bh * scale_y;

            if (line_no == cursor[0]+1 and ch_no == cursor[1])
            {
                cursor_pos_x = x;
                cursor_pos_x_p = x_p;
            }
            x_p += a->c[ch].ax;
            x += a->c[ch].ax * scale_x;
            y += a->c[ch].ay * scale_y;


            ch_no++;

            if (!w || !h)
                continue;

            data[n++] = (point) {xpos, ypos + h, a->c[ch].tx, 0.0f};
            data[n++] = (point) {xpos, ypos , a->c[ch].tx, a->c[ch].bh/a->h};
            data[n++] = (point) {xpos + w, ypos, a->c[ch].tx + (a->c[ch].bw/a->w), a->c[ch].bh/a->h};

            data[n++] = (point) {xpos, ypos + h, a->c[ch].tx, 0.0f};
            data[n++] = (point) {xpos + w, ypos, a->c[ch].tx + (a->c[ch].bw/a->w), a->c[ch].bh/a->h};
            data[n++] = (point) {xpos + w, ypos + h, a->c[ch].tx + (a->c[ch].bw/a->w), 0.0f}; 
        }
        if (line_no == cursor[0]+1 and ch_no == cursor[1])
        {
            cursor_pos_x = x; //last character
            cursor_pos_x_p = x_p;
        }
        if (view_bounds_y[0] == -1 and y < 1)
        {
            view_bounds_y[0] = line_no;
        }
        if (y > -1)
        {
            view_bounds_y[1] = line_no;
        }
        xmax = std::max(x_p, xmax);
        
        line_no++;
        y = 1 - (line_no*(a->h+line_spacing)*scale_y) -((start_pos_y+scroll_y)*scale_y);
        x = -1 + (start_pos_x+scroll_x)*scale_x;
    }
    glBufferData(GL_ARRAY_BUFFER,sizeof(data), data, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, n);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
}

void Renderengine::render_cursor()
{
    glBindVertexArray(cursor_VAO);
    glBindBuffer(GL_ARRAY_BUFFER,cursor_VBO);
    int line_no = cursor[0] + 1;
    float x = cursor_pos_x;
    float y = 1 - (line_no*(a->h+line_spacing)*scale_y) -((start_pos_y+scroll_y)*scale_y) -((a->h*0.25)*scale_y);
    float data[2*6];
    float cursor_height = a->h + (a->h*0.15); // in pixels
    int cursor_width = 2; // in pixels

    data[0] = x; data[1] = y;
    data[2] = x; data[3] = y + cursor_height*scale_y;
    data[4] = x + cursor_width*scale_x; data[5] = y;

    data[6] = x; data[7] = y + cursor_height*scale_y;
    data[8] = x + cursor_width*scale_x; data[9] = y;
    data[10] = x + cursor_width*scale_x; data[11] = y + cursor_height*scale_y;


    glBufferData(GL_ARRAY_BUFFER,sizeof(data), data, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    
}

void Renderengine::set_scroll(float yoffset, float xoffset)
{
    scroll_y = yoffset*(a->h+line_spacing);
    scroll_x = -xoffset;
}

void Renderengine::draw()
{
    glClear(GL_COLOR_BUFFER_BIT);
    text_shader.use();
    glUniform3f(glGetUniformLocation(text_shader.ID,"textColor"),txtcolor[0],txtcolor[1],txtcolor[2]);
    render_text();
    cursor_shader.use();
    float timeValue = glfwGetTime();
    float blink_speed = 5;
    float cursor_alpha = (std::sin(timeValue*blink_speed) / 2.0f) + 0.5f;
    cursor_alpha = std::round(cursor_alpha);
    glUniform4f(glGetUniformLocation(cursor_shader.ID,"cursorColor"),cursorcolor[0],cursorcolor[1],cursorcolor[2],cursor_alpha);
    render_cursor();
}


#endif
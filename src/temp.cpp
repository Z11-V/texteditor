#include <ft2build.h>
#include FT_FREETYPE_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "shader.h"
#include "textbuffer.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

unsigned int text_VAO,text_VBO;
float textsx;
float textsy;
std::string text_buffer = "";
int cursor = 0;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void window_refresh_callback(GLFWwindow *window);
void character_callback(GLFWwindow* window, unsigned int codepoint);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);
void draw();

struct atlas
{
    unsigned int texture;
    unsigned int w; //width of texture
    unsigned int h; //height of texture

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

atlas* a12;
atlas* a24;
atlas* a48;

void renderText(std::string text, atlas *a, float x, float y, float sx, float sy)
{
    glBindTexture(GL_TEXTURE_2D, a->texture);
    glBindVertexArray(text_VAO);
    glBindBuffer(GL_ARRAY_BUFFER,text_VBO);

    struct point
    {
        float x;
        float y;
        float s;
        float t;
    }data[6*text.length() + 6];

    int n = 0; //no of points
    int line_n = 1;
    int chpos = 0; // character position

    for (char& ch : text)
    {
        //cursor
        if (cursor == chpos)
        {
            data[n++] = (point) {x,y + 40*sy,-1,-1};
            data[n++] = (point) {x,y,-1,-1};
            data[n++] = (point) {x+2*sx,y,-1,-1};

            data[n++] = (point) {x,y + 40*sy,-1,-1};
            data[n++] = (point) {x+2*sx,y,-1,-1};
            data[n++] = (point) {x+2*sx,y + 40*sy,-1,-1};
        }
        chpos++;
        
        //text
        float xpos = x + a->c[ch].bl * sx;
        float ypos = y -(a->c[ch].bh - a->c[ch].bt) * sy;
        

        float w = a->c[ch].bw * sx;
        float h = a->c[ch].bh * sy;

        x += a->c[ch].ax * sx;
        y += a->c[ch].ay * sy;

        if (ch == '\n')
        {
            line_n++;
            y = 1 - line_n*48*sy;
            x = -1 + 0*sx;
        }
        

        if (!w || !h)
            continue;

        data[n++] = (point) {xpos, ypos + h, a->c[ch].tx, 0.0f};
        data[n++] = (point) {xpos, ypos , a->c[ch].tx, a->c[ch].bh/a->h};
        data[n++] = (point) {xpos + w, ypos, a->c[ch].tx + (a->c[ch].bw/a->w), a->c[ch].bh/a->h};

        data[n++] = (point) {xpos, ypos + h, a->c[ch].tx, 0.0f};
        data[n++] = (point) {xpos + w, ypos, a->c[ch].tx + (a->c[ch].bw/a->w), a->c[ch].bh/a->h};
        data[n++] = (point) {xpos + w, ypos + h, a->c[ch].tx + (a->c[ch].bw/a->w), 0.0f};

        
    }

    glBufferData(GL_ARRAY_BUFFER,sizeof(data), data, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, n);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
}


int init_atlas()
{
    FT_Library ft;

    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return 0;
    }
    
    FT_Face face;
    if (FT_New_Face(ft, "fonts/OpenSans-Light.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;  
        return 0;
    }

    a12 = new atlas(face, 12);
    a24 = new atlas(face,24);
    a48 = new atlas(face,48);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return 1;

}

int main()
{
    //glfw : initialization
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    
    //glfw : window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Text", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCharCallback(window, character_callback);
    glfwSetKeyCallback(window, key_callback);
    //glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    //glfwSetWindowRefreshCallback(window, window_refresh_callback);
    //glfwSwapInterval(0);

    //glad : load opengl functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    Shader shader("shaders/text.vert","shaders/text.frag");
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    
    init_atlas();
    glGenVertexArrays(1, &text_VAO);
    glGenBuffers(1, &text_VBO);
    glBindVertexArray(text_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, text_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*6*4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(float),0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    //text
    textsx = 2.0/800;
    textsy = 2.0/600;
    std::ifstream txtfile("test.txt");
    if (txtfile.is_open())
    {
        std::stringstream ss;
        ss <<  txtfile.rdbuf();
        txtfile.close();
        text_buffer = ss.str();
    } else
    {
        text_buffer = "Hello World";
    }
    
    
    //render loop
    while (!glfwWindowShouldClose(window))
    {
        //input
        processInput(window);

        //rendering
        shader.use();
        glUniform3f(glGetUniformLocation(shader.ID,"textColor"),1.0f,0.5f,0.2f);
        draw();
        glfwSwapBuffers(window);

        //check events
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

void draw()
{
    glClear(GL_COLOR_BUFFER_BIT);
    renderText(text_buffer,a48,-1+0*textsx,1-48*textsy,textsx,textsy);
    //renderText("Hello World 2",a48,-1+0*textsx,1-2*48*textsy,textsx,textsy);
}

void character_callback(GLFWwindow* window, unsigned int character)
{
    std::cout<<cursor;
    text_buffer.insert(cursor, 1, (char)character);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ENTER && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        text_buffer.insert(cursor, 1, '\n');
    }
    if (key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        if (!text_buffer.empty())
        {
            text_buffer.pop_back();
        } 
    }
    if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        cursor++;
    }
    if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        cursor--;
    }
    
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    textsx = 2.0/width;
    textsy = 2.0/height;
    draw();
    glfwSwapBuffers(window);
    glFinish();
}
void window_refresh_callback(GLFWwindow *window)
{
    draw();
    glfwSwapBuffers(window);
    glFinish();
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
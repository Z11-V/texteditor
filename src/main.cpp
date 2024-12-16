#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <array>
#include <nfd.hpp>
#include "atlas.h"
#include "textbuffer.h"
#include "renderengine.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 630;
const std::string title("Notebad");

Textbuffer textbuffer{};

float cursor_pos_x{0}; // in terms of pixels
float scrollval_y{0}; // in terms of lines (need to change to pixels)
std::array<int,2> view_bounds_y{};
float ymax{};
float scrollval_x{0}; // in terms of pixels
float xmax{};
bool queue_adjust_cursor = false;

void processInput(GLFWwindow *window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void character_callback(GLFWwindow* window, unsigned int character);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void scroll_y(float yoffset);
void scroll_x(float xoffset);
void adjust_cursor_y();
void adjust_cursor_x();

std::string currentfile;
void save()
{
    if (currentfile.empty())
    {
        // no current file
        NFD::Guard nfdGuard;
        NFD::UniquePath outPath;
        nfdfilteritem_t filterItem[1] = {{"Text Files", "txt"}};

        nfdresult_t result = NFD::SaveDialog(outPath, filterItem, 1);
        if (result == NFD_OKAY) {
            std::cout << "Save file selected" << std::endl;
            currentfile = outPath.get();
        } else if (result == NFD_CANCEL) {
            std::cout << "User pressed cancel." << std::endl;
        } else {
            std::cout << "Error: " << NFD::GetError() << std::endl;
        }
    }
    if (!currentfile.empty())
    {
        textbuffer.write_to_file(currentfile);
        std::cout << "File Saved" << std::endl;
    }
    
}

void open()
{
    if (currentfile.empty())
    {
        // no current file
        NFD::Guard nfdGuard;
        NFD::UniquePath outPath;
        nfdfilteritem_t filterItem[1] = {{"Text Files", "txt"}};

        nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 1);
        if (result == NFD_OKAY) {
            std::cout << "File selected" << std::endl;
            currentfile = outPath.get();
            textbuffer.read_from_file(currentfile);
        } else if (result == NFD_CANCEL) {
            std::cout << "User pressed cancel." << std::endl;
        } else {
            std::cout << "Error: " << NFD::GetError() << std::endl;
        }
    }
    else
    {
        std::cout << "Can't open another file" << std::endl;
    }
    
}

int main()
{
    //initialization
    //glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, title.c_str(), NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, character_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //atlas
    FT_Library ft;

    if (FT_Init_FreeType(&ft))
    {
        std::cout << "Could not init FreeType Library" << std::endl;
        return -1;
    }
    
    FT_Face face;
    if (FT_New_Face(ft, "fonts/OpenSans-Light.ttf", 0, &face))
    {
        std::cout << "Failed to load font" << std::endl;  
        return -1;
    }

    atlas a12(face, 12);
    atlas a24(face, 24);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    

    Renderengine engine(SCR_WIDTH, SCR_HEIGHT, &a24, textbuffer.cursor, textbuffer.view, textbuffer.size);
    

    //main loop
    while (!glfwWindowShouldClose(window))
    {
        //input
        processInput(window);

        engine.set_scroll(scrollval_y,scrollval_x);
        engine.draw();
        view_bounds_y = engine.view_bounds_y;
        xmax = engine.xmax;
        cursor_pos_x = engine.cursor_pos_x_p;

        if (queue_adjust_cursor)
        {
            adjust_cursor_y();
            adjust_cursor_x();
            queue_adjust_cursor = false;
            glfwSetTime(0);
        }
        

        glfwSwapBuffers(window);

        glfwPollEvents();
    }
    //save();
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void scroll_y(float yoffset)
{
    scrollval_y += yoffset;
    ymax = -textbuffer.no_of_lines+1;
    if (scrollval_y >= 0)
    {
        scrollval_y = 0;
    }
    if (scrollval_y <= ymax)
    {
        scrollval_y = ymax;
    }   
}

void scroll_x(float xoffset)
{
    scrollval_x += xoffset; // +ve number
    float limit = std::max(0.0f, xmax - SCR_WIDTH);
    scrollval_x = std::clamp(scrollval_x, 0.0f, limit);
}

void adjust_cursor_y()
{
    //adjust scroll y
    int y = textbuffer.cursor[0]+1;
    if (view_bounds_y[0] > y)
    {
        scroll_y(view_bounds_y[0] - y);
    }
    if (y > view_bounds_y[1])
    {
        scroll_y(view_bounds_y[1] - y);
    }
}

void adjust_cursor_x()
{
    //adjust scroll x
    if (cursor_pos_x < scrollval_x or cursor_pos_x > (scrollval_x+SCR_WIDTH))
    {
        int diff = cursor_pos_x - (scrollval_x + SCR_WIDTH);
        scroll_x(diff);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        scroll_x(yoffset*25);
    else
        scroll_y(yoffset);
}

void character_callback(GLFWwindow* window, unsigned int character)
{
    textbuffer.insert_character((char)character);
    queue_adjust_cursor = true;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){

    //arrow keys
    if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        if (mods == GLFW_MOD_SHIFT)
        {
            textbuffer.move_cursor(CURSOR_LEND);
        }
        else
        {
            textbuffer.move_cursor(CURSOR_RIGHT);
        }
        queue_adjust_cursor = true;
    }
    if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        if (mods == GLFW_MOD_SHIFT)
        {
            textbuffer.move_cursor(CURSOR_LSTART);
        }
        else
        {
            textbuffer.move_cursor(CURSOR_LEFT);
        }
        queue_adjust_cursor = true;
    }
    if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        textbuffer.move_cursor(CURSOR_UP);
        queue_adjust_cursor = true;
    }
    if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        textbuffer.move_cursor(CURSOR_DOWN);
        queue_adjust_cursor = true;
    }

    //enter and backspace

    if (key == GLFW_KEY_ENTER && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        textbuffer.newline();
        queue_adjust_cursor = true;
    }

    if (key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        textbuffer.backspace();
        queue_adjust_cursor = true;
        adjust_cursor_y(); // needed to prevent flicker
    }

    if (key == GLFW_KEY_S && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL )
    {
        save();
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL )
    {
        open();
    }
}

#ifndef TEXTBUFFER_H
#define TEXTBUFFER_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <string_view>
#include <array>
#include <algorithm>

enum Cursor_movement { CURSOR_UP, CURSOR_DOWN, CURSOR_LEFT, CURSOR_RIGHT, CURSOR_LSTART, CURSOR_LEND};

class Textbuffer
{
private:
    void generate_view();
public:
    std::vector<std::string> buffer;
    int no_of_lines;
    int size;
    std::vector<std::string_view> view;
    std::array<int,2> cursor = {0,0}; //line_no(from zero),character offset
    Textbuffer();
    ~Textbuffer();
    
    void read_from_file(std::string filename);
    void write_to_file(std::string filename);
    void move_cursor(Cursor_movement move);
    void insert_character(char ch);
    void backspace();
    void newline();
};

void Textbuffer::read_from_file(std::string filename)
{
    std::ifstream textfile(filename);
    std::string line;
    buffer.clear();
    if (textfile.is_open())
    {
        while (std::getline(textfile, line))
        {
            buffer.push_back(line);
        }
        
    }
    else
    {
        std::cout<<"Failed to load file"<<std::endl;
        buffer.push_back("");
    }
    cursor[0] = 0; //reset cursor
    cursor[1] = 0;
    generate_view();
}

void Textbuffer::write_to_file(std::string filename)
{
    std::ofstream textfile(filename);
    if (textfile.is_open())
    {
        for (auto line : buffer)
        {
            textfile<<line<<std::endl;
        }
    }
    else
    {
        std::cout<<"Unable to write to file"<<std::endl;
    }
    
}

Textbuffer::Textbuffer()
{
    buffer.push_back("");
    generate_view();
}

Textbuffer::~Textbuffer()
{
}

void Textbuffer::generate_view()
{
    view.clear();
    no_of_lines = buffer.size();
    int temp_size = 0;
    for (size_t i = 0; i < no_of_lines; i++)
    {
        view.push_back(buffer.at(i));
        temp_size += buffer.at(i).length();
    }
    size = temp_size;
}

void Textbuffer::move_cursor(Cursor_movement move)
{
    switch (move)
    {
    case CURSOR_UP:
        if (cursor[0] !=0)
        {
            cursor[0]--;
            cursor[1] = std::clamp(cursor[1],0,(int)buffer[cursor[0]].length());
        }
        else
        {
            //top of file
            cursor[1] = 0;
        }
        //std::cout<<"("<<cursor[0]<<","<<cursor[1]<<")"<<" ";
        break;
    case CURSOR_DOWN:
        if (cursor[0] != no_of_lines-1)
        {
            cursor[0]++;
            cursor[1] = std::clamp(cursor[1],0,(int)buffer[cursor[0]].length());
        }
        else
        {
            //bottom of file
            cursor[1] = buffer[cursor[0]].length();
        }
        break;
    case CURSOR_LEFT:
        if (cursor[1] !=0)
        {
            cursor[1]--;
        }
        break;
    case CURSOR_RIGHT:
        if (cursor[1] != buffer[cursor[0]].length())
        {
            cursor[1]++;
        }
        break;
    case CURSOR_LSTART:
        cursor[1] = 0;
        break;
    case CURSOR_LEND:
        cursor[1] = buffer[cursor[0]].length();
        break;
    default:
        break;
    }
}

void Textbuffer::insert_character(char ch)
{
    buffer[cursor[0]].insert(cursor[1], 1, ch);
    generate_view();
    move_cursor(CURSOR_RIGHT);
}

void Textbuffer::backspace()
{   
    if (cursor[1] == 0)
    {
        if (cursor[0] == 0)
        {
            return;  
        }
        auto t = buffer.at(cursor[0]);
        buffer.erase(buffer.begin()+cursor[0]);
        move_cursor(CURSOR_UP);
        move_cursor(CURSOR_LEND);
        buffer[cursor[0]].append(t);
        generate_view();
        return;
    }
    buffer[cursor[0]].erase(cursor[1]-1,1);
    generate_view();
    move_cursor(CURSOR_LEFT);
}

void Textbuffer::newline()
{
    auto t = buffer[cursor[0]].substr(cursor[1]);
    buffer.insert(buffer.begin()+cursor[0]+1,t);
    buffer[cursor[0]].erase(cursor[1]);
    generate_view();
    move_cursor(CURSOR_DOWN);
    move_cursor(CURSOR_LSTART);
}

#endif

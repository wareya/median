// Simple image stuff helper library. Use --std=c++11
//
// Intended for use for writing small image testing programs.
//
// Here are some examples of stuff to do with this:
// 1) Write a chroma plane generator
// 2) Test experimental image filters
// 3) Delete every other scanline from interlaced pixel art
// 4) Send a .ppm image to your boss because you want to troll them
// 5) Generate a fractal

/*
   Copyright 2016 Alexander "wareya" Nadeau <wareya@gmail.com>

Unlicensed

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

*/

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h> // memcmp

#include <vector>
#include <string>
#include <iostream>

#include "endian.h"

// Convert 0.0~1.0 to 0~255 with correct clipping and rounding.
uint8_t fix(float capme)
{
    capme *= 255;
    capme += 0.5; // for rounding around truncating conversion to int
    if(capme < 0)
        return 0;
    if(capme > 255)
        return 255;
    return (int8_t)capme;
}

float tolinear(float srgb)
{
    if(srgb > 0.04045)
        return pow((srgb + 0.055)/1.055, 2.4);
    else
        return srgb/12.92;
}
float tosrgb(float linear)
{
    if(linear > 0.0031308)
        return 1.055*pow(linear, 1/2.4) - 0.055;
    else
        return linear*12.92;
}
// http://entropymine.com/imageworsener/srgbformula/
// Misnomer: actually better!
float tolinear_worse(float srgb)
{
    if(srgb > 0.0404482362771082)
        return pow((srgb + 0.055)/1.055, 2.4);
    else
        return srgb/12.92;
}
float tosrgb_worse(float linear)
{
    if(linear > 0.00313066844250063)
        return 1.055*pow(linear, 1/2.4) - 0.055;
    else
        return linear*12.92;
}

struct triad
{
    float r;
    float g;
    float b;
    
    triad()
    {
        r = 1.0;
        g = 1.0;
        b = 1.0;
    }
    triad(float ar, float ag, float ab)
    {
        r = ar;
        g = ag;
        b = ab;
    }
    bool operator<(triad o) // for std::sort
    {
        if(r+g+b < o.r+o.g+o.b)
            return true;
        return false;
    }
    triad operator+(triad o)
    {
        return triad(r+o.r, g+o.g, b+o.b);
    }
    triad operator*(float f)
    {
        return triad(r*f, g*f, b*f);
    }
};
// Made-up convention. Sorry.
struct criad
{
    float y;
    float g;
    float o;
    
    criad()
    {
        y = 1.0;
        g = 0.5;
        o = 0.5;
    }
    criad(float ay, float ag, float ao)
    {
        y = ay;
        g = ag;
        o = ao;
    }
};

// https://en.wikipedia.org/wiki/Line–line_intersection#Given_two_points_on_each_line
// simplified where x1,y1 == 0,0; subscripts rotated down by 1
void project(float x1, float y1, float x2, float y2, float x3, float y3, float& xout, float& yout)
{
    float denominator = ((-x1)*(y2-y3)-(-y1)*(x2-x3));
    if(denominator == 0)
    {
        xout = x1;
        yout = y1;
        return;
    }
    xout = ((y1-x1)*(x2-x3)-(-x1)*(x2*y3-y2*x3))/((-x1)*(y2-y3)-(-y1)*(x2-x3));
    yout = ((y1-x1)*(y2-y3)-(-y1)*(x2*y3-y2*x3))/denominator;
}

// I have not tested this!!!! Be careful!
triad ygo_to_srgb(criad ygo)
{
    if(ygo.y >= 1.0)
        return triad({1,1,1});
    if(ygo.y <= 0)
        return triad({0,0,0});
    if(ygo.g == 0.5 and ygo.o == 0.5)
        return triad({ygo.y,ygo.y,ygo.y});
    
    // Constraints!
    // Y 0~1
    // C -0.5~0.5
    //: G > -Y
    //: G+O < Y
    //: G-O < Y
    //: G < 1-Y
    //: G+O > Y-1
    //: G-O > Y-1
    
    /* The [C -0.5~0.5] case is handled incidentally by the combination of all six other restrictions!
       This is an example of what it would look like. Only the code for the G channel.
    if (ygo.g < -0.5)
        project(ygo.g, ygo.o, -0.5, -0.5, -0.5, 0.5, ygo.g, ygo.o);
    if (ygo.g > -0.5)
        project(ygo.g, ygo.o, 0.5, 0.5, -0.5, 0.5, ygo.g, ygo.o);
    */
    
    if(ygo.g > -ygo.y)
        project(ygo.g, ygo.o, -ygo.y, -0.5, -ygo.y,    0.5, ygo.g, ygo.o);
    if(ygo.g+ygo.o < ygo.y)
        project(ygo.g, ygo.o,  ygo.y,    0,      0,  ygo.y, ygo.g, ygo.o);
    if(ygo.g-ygo.o < ygo.y)
        project(ygo.g, ygo.o,  ygo.y,    0,      0, -ygo.y, ygo.g, ygo.o);
    
    if(ygo.g < 1-ygo.y)
        project(ygo.g, ygo.o,  ygo.y, 0.5, ygo.y,   -0.5, ygo.g, ygo.o);
    if(ygo.g+ygo.o > ygo.y-1)
        project(ygo.g, ygo.o, -ygo.y,   0,     0, -ygo.y, ygo.g, ygo.o);
    if(ygo.g-ygo.o > ygo.y-1)
        project(ygo.g, ygo.o, -ygo.y,   0,     0,  ygo.y, ygo.g, ygo.o);
    
    triad rgb;
    rgb.r = ygo.y - ygo.g + ygo.o;
    rgb.g = ygo.y + ygo.g;
    rgb.b = ygo.y - ygo.g - ygo.o;
    return rgb;
}


struct image
{
    unsigned int width;
    unsigned int height;
    std::vector<triad> data;
    
    image()
    {
        width = 1;
        height = 1;
        data = {triad()};
    }
    
    triad& operator()(unsigned int x, unsigned int y)
    {
        return data[x+width*y];
    }
    
    void dimensions(unsigned int arg_width, unsigned int arg_height)
    {
        width = arg_width;
        height = arg_height;
        data = std::vector<triad>();
        data.resize(width*height);
    }
    
    void makelinear()
    {
        for(auto& t : data)
        {
            t.r = tolinear(t.r);
            t.g = tolinear(t.g);
            t.b = tolinear(t.b);
        }
    }
    void makesrgb()
    {
        for(auto& t : data)
        {
            t.r = tosrgb(t.r);
            t.g = tosrgb(t.g);
            t.b = tosrgb(t.b);
        }
    }
    void makelinear_worse()
    {
        for(auto& t : data)
        {
            t.r = tolinear_worse(t.r);
            t.g = tolinear_worse(t.g);
            t.b = tolinear_worse(t.b);
        }
    }
    void makesrgb_worse()
    {
        for(auto& t : data)
        {
            t.r = tosrgb_worse(t.r);
            t.g = tosrgb_worse(t.g);
            t.b = tosrgb_worse(t.b);
        }
    }
    void applygamma(float g)
    {
        for(auto& t : data)
        {
            t.r = pow(t.r, g);
            t.g = pow(t.g, g);
            t.b = pow(t.b, g);
        }
    }
    
    void writeppm(const char * filename)
    {
        std::string temp(filename);
        if(temp.size() <= 4)
        {
            puts("fixing filename");
            temp += ".ppm";
            filename = temp.data();
        }   // Unfortunately I have to duplicate code or else use gotos or logic variables.
        if(temp.substr(temp.length()-4) != ".ppm")
        {
            puts("fixing filename");
            temp += ".ppm";
            filename = temp.data();
        }
        
        FILE* file = fopen(filename, "wb");
        printf("writing file %s\n", filename);
        if(file != NULL)
        {
            printf("w h : %d %d\n", width, height);
            fprintf(file, "P6 %d %d 255\n", width, height);
            for(auto& t : data)
            {
                fputc(fix(t.r), file);
                fputc(fix(t.g), file);
                fputc(fix(t.b), file);
            }
        
            fclose(file);
        }
        else
            puts("Error opening file.");
    }
    void readff(const char * filename)
    {
        std::string temp(filename);
        if(temp.size() <= 3)
        {
            puts("fixing filename");
            temp += ".ff";
            filename = temp.data();
        }   // Unfortunately I have to duplicate code or else use gotos or logic variables.
        else if(temp.substr(temp.length()-3) != ".ff")
        {
            puts("fixing filename");
            temp += ".ff";
            filename = temp.data();
        }
        
        FILE* file = fopen(filename, "rb");
        printf("reading file %s\n", filename);
        if(file != NULL)
        {
            char name[8];
            fread(name, 1, 8, file);
            printf("%.8s -- header magic\n", name);
            if(memcmp(name, "farbfeld", 8) == 0)
            {
                {
                    unsigned int scratch[2];
                    fread(scratch, 4, 2, file);
                    width = swap32(scratch[0]);
                    height = swap32(scratch[1]);
                    
                    std::cout << width << " " << height << " -- dimensions\n";
                    
                    dimensions(width, height);
                }
                for(auto& t : data)
                {
                    unsigned short scratch[4];
                    fread(scratch, 2, 4, file);
                    t.r = swap16(scratch[0])*1.0/0xFFFF;
                    t.g = swap16(scratch[1])*1.0/0xFFFF;
                    t.b = swap16(scratch[2])*1.0/0xFFFF;
                }
            }
            else
                puts("Not a valid farbfeld file.");
        
            fclose(file);
            
            std::cout << data.size() << " -- number of pixels in farbfeld\n";
        }
        else
            puts("Error opening file.");
    }
};

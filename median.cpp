// compile with --std=c++11

#include "helper.cpp" 

#include <algorithm>

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

// Denoise-dering an image using a weighted median.

int main(int argc, const char* argv[])
{
    if(argc == 1 or strcmp(argv[1], "--help") == 0 or strcmp(argv[1], "-h") == 0)
    {
        puts("Usage: median <filename> [--srgb] [--blurry|--blurrier|--special]");
        puts("<filename> must be a farbfeld image file with the .ff extension present.");
        puts("If <filename> has an extension, the output will contain it: 'fab.ff.ppm'");
        puts("The output filename uses the input filename with the ppm file extension.");
        puts("");
        puts("All filtering is done in linear RGB by default. Add '--srgb' immediately");
        puts("after the input filename to filter in sRGB gamma. Sometimes fixes moire.");
        puts("");
        puts("'--blurry' makes the filter blend multiple kernel pixel values together.");
        puts("It's nearly invisible, but *does* smooth certain features very slightly.");
        puts("");
        puts("'--blurrier' is like '--blurry', but blends more tentative pixel values.");
        puts("It's much stronger, and can make or break the filtering of some artwork.");
        puts("");
        puts("'--special' does not weight the median but instead blurs the median set.");
        puts("The median set is blurred with a triangle kernel that catches the edges.");
        puts("This is done after sorting, so it's different from straightforward blur.");
        puts("It's almost as sharp as a 2x2 blur but with less noise, and is centered.");
        puts("");
        puts("ppm is a very old text-based image format that is very easy to generate.");
        puts("For software that can open ppm images, I use KolourPaint, a Paint clone.");
        puts("");
        puts("farbfeld is a binary image format. It is very extremely easy to process.");
        puts("For software for using farbfeld, see http://tools.suckless.org/farbfeld/");
        return 0;
    }
    image img;
    img.readff(argv[1]);
    image dest;
    dest.dimensions(img.width, img.height);
    
    if(img.width * img.height == 1)
    {
        puts("Nothing to do. Image is only one pixel large. Output not written.");
        return 0;
    }
    
    puts("Running median");
    
    int blurry = 0;
    bool dolinear = true;
    int n = 3;
    if(argc >= n)
    {
        if(strcmp(argv[n-1], "--srgb") == 0)
        {
            puts("Not using linear RGB.");
            dolinear = false;
            n += 1;
        }
    }
    if(argc >= n)
    {
        if(strcmp(argv[n-1], "--blurry") == 0)
        {
            blurry = 1;
            puts("Blurry mode.");
        }
        else if(strcmp(argv[n-1], "--blurrier") == 0)
        {
            blurry = 2;
            puts("Blurrier mode.");
        }
        else if(strcmp(argv[n-1], "--special") == 0)
        {
            blurry = 3;
            puts("Special mode.");
        }
    }
    
    if(dolinear)
        img.makelinear_worse();
    
    for(unsigned int x = 0; x < img.width; x++)
    {
        for(unsigned int y = 0; y < img.height; y++)
        {
            std::vector<triad> testpixels;
            // Kernel: 121 \n 242 \n 121
            // Implemented by duplication.
            auto push = [&](long ax, long ay)
            {
                if(ax >= 0 and ax < img.width and ay >= 0 and ay < img.height)
                    testpixels.push_back(img(ax,ay));
            };
            
            push(x-1, y-1);
            push(x-1, y+1);
            push(x+1, y+1);
            push(x+1, y-1);
            
            push(x-1, y);
            push(x-1, y);
            push(x+1, y);
            push(x+1, y);
            push(x, y+1);
            push(x, y+1);
            push(x, y-1);
            push(x, y-1);
            
            push(x, y);
            push(x, y);
            push(x, y);
            push(x, y);
            
            std::sort(testpixels.begin(), testpixels.end());
            
            // A one-dimensional image with at least two pixels has a minimum kernel size of two pixels: center and side.
            // Sides are weighted at 2, and center is weighted at 4. We shouldn't run this cout statement.
            // If we do, something broke very horribly.
            if(testpixels.size() < 6)
                std::cout << testpixels.size() << " " << x << " " << y << " -- kernelsize, x, y \n";
            
            if(blurry < 3)
            {
                if((testpixels.size()&1) == 1)
                {
                    auto mid = (testpixels.size()-1)/2;
                    if(blurry == 0)
                    {
                        dest(x,y) = testpixels[mid];
                    }
                    if(blurry == 1)
                    {
                        // 3 width
                        dest(x,y) = (testpixels[mid-1]
                                    +testpixels[mid  ]
                                    +testpixels[mid+1])*(1.0/3);
                    }
                    if(blurry == 2)
                    {
                        // 5 width
                        dest(x,y) = (testpixels[mid-2]
                                    +testpixels[mid-1]
                                    +testpixels[mid  ]
                                    +testpixels[mid+1]
                                    +testpixels[mid+2])*(1.0/5);
                    }
                }
                else // even number of cells
                {
                    auto topmid = testpixels.size()/2;
                    if(blurry == 0)
                    {
                        // 2 width
                        dest(x,y) = (testpixels[topmid-1]
                                    +testpixels[topmid  ])*0.5;
                    }
                    if(blurry == 1)
                    {
                        // 4 width
                        dest(x,y) = (testpixels[topmid-2]
                                    +testpixels[topmid-1]
                                    +testpixels[topmid  ]
                                    +testpixels[topmid+1])*0.25;
                    }
                    if(blurry == 2)
                    {
                        // 6 width
                        dest(x,y) = (testpixels[topmid-3]
                                    +testpixels[topmid-2]
                                    +testpixels[topmid-1]
                                    +testpixels[topmid  ]
                                    +testpixels[topmid+1]
                                    +testpixels[topmid+2])*(1.0/6);
                    }
                }
            }
            else if (blurry == 3)
            {
                float mid = (testpixels.size()-1)/2.0;
                triad scrap(0,0,0);
                float normalize = 0;
                for(unsigned i = 0; i < testpixels.size(); i++)
                {
                    float factor = mid-fabs(i-mid);
                    factor += 1;
                    scrap += testpixels[i]*factor;
                    normalize += factor;
                }
                dest(x,y) = scrap*(1/normalize);
            }
            
            /*
            Old code that made sure to specifically gaussian blur things together.
            The new code uses an averaging filter due to the realization that the
             gaussian distribution of the already sorted list already strengthens
             the center pixel quite a bit. Averaging somehow gives better results.
            Here for reference.
            if((testpixels.size()&1) == 1)
            {
                auto mid = (testpixels.size()-1)/2;
                if(blurry == 0)
                {
                    dest(x,y) = testpixels[mid];
                }
                if(blurry == 1)
                {
                    // 3 width
                    // 1/4, 1/2, 1/4
                    dest(x,y) = testpixels[mid]*0.5;
                    dest(x,y) += (testpixels[mid-1]+testpixels[mid+1])*0.25;
                }
                if(blurry == 2)
                {
                    // 5 width
                    // 1/16, 4/16, 6/16, 4/16, 1/16
                    dest(x,y) = testpixels[mid]*0.375;
                    dest(x,y) += (testpixels[mid-1]+testpixels[mid+1])*0.25;
                    dest(x,y) += (testpixels[mid-2]+testpixels[mid+2])*0.0625;
                }
            }
            else
            {
                auto topmid = testpixels.size()/2;
                if(blurry == 0)
                {
                    dest(x,y) = (testpixels[topmid-1]+testpixels[topmid])*0.5;
                }
                if(blurry == 1)
                {
                    // 4 width
                    // 1/8, 3/8, 3/8, 1/8
                    dest(x,y) = (testpixels[topmid-1]+testpixels[topmid])*0.375;
                    dest(x,y) += (testpixels[topmid-2]+testpixels[topmid+1])*0.125;
                }
                if(blurry == 2)
                {
                    // 6 width
                    // 1/32 5/32 10/32 10/32 5/32 1/32
                    dest(x,y) = (testpixels[topmid-1]+testpixels[topmid])*0.3125;
                    dest(x,y) += (testpixels[topmid-2]+testpixels[topmid+1])*0.15625;
                    dest(x,y) += (testpixels[topmid-3]+testpixels[topmid+2])*0.03125;
                }
            }*/
        }
    }
    puts("Done.");
    
    if(dolinear)
        dest.makesrgb_worse();
    
    dest.writeppm(argv[1]);
}

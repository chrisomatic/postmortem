#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GL/glew.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG 
#include "util/stb_image.h"

#include "common.h"
#include "shader.h"
#include "gfx.h"

#define MAX_GFX_IMAGES 32

static GLuint texture = 0;
static GLuint vao, vbo, ibo;

static uint32_t* back_buffer;
static uint32_t buffer_width;
static uint32_t buffer_height;

void gfx_init(int width, int height)
{
    printf("GL version: %s\n",glGetString(GL_VERSION));

    glClearColor(0.15f, 0.15f, 0.15f, 0.0f);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    float z = 0.9;

    Vertex vertices[4] = 
    {
        {{-z, -z},{+0.0,+0.0}},
        {{-z, +z},{+0.0,+1.0}},
        {{+z, +z},{+1.0,+1.0}},
        {{+z, -z},{+1.0,+0.0}},
    }; 

    uint32_t indices[6] = {0,1,2,2,0,3};

 	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 4*sizeof(Vertex), vertices, GL_STATIC_DRAW);

    glGenBuffers(1,&ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(uint32_t), indices, GL_STATIC_DRAW);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glUniform1i(glGetUniformLocation(program, "sampler"), 0);

    glBindTexture(GL_TEXTURE_2D, 0);

    back_buffer = malloc(width*height*sizeof(uint32_t));
    buffer_width = width;
    buffer_height = height;

    stbi_set_flip_vertically_on_load(1);
}

void gfx_clear_buffer(uint32_t color)
{
    int buffer_len = buffer_width*buffer_height;
    for(int i = 0; i < buffer_len; ++i)
        back_buffer[i] = color;
}

void gfx_draw()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);

    //glEnable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, buffer_width, buffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, back_buffer);

    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vertex), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex), (const GLvoid*)8);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo);

    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);
}

typedef struct
{
    unsigned char* data;
    int w,h,n;
} GFXImage;

GFXImage gfx_images[MAX_GFX_IMAGES] = {0};
int gfx_image_count = 0;

int gfx_load_image(const char* image_path)
{
    for(int i = 0; i < MAX_GFX_IMAGES; ++i)
    {
        if(gfx_images[i].data == NULL)
        {
            GFXImage* img = &gfx_images[i];
            img->data = stbi_load(image_path,&img->w,&img->h,&img->n,4);

            if(img->data == NULL)
                return -1;

            printf("Loaded image: %s (x: %d, y: %d, n: %d)\n",image_path,img->w,img->h,img->n);
            return i;
        }
    }

    return -1;
}

void gfx_free_image(int img_index)
{
    GFXImage* img = &gfx_images[img_index];
    stbi_image_free(img->data);
    memset(img, 0, sizeof(GFXImage));
}

bool gfx_draw_image(int img_index, int x, int y,float scale)
{
    if(img_index < 0)
    {
        printf("Invalid image index!\n");
        return false;
    }
    GFXImage* img = &gfx_images[img_index];

    int dst_w = (int)(img->w*scale);
    int dst_h = (int)(img->h*scale);
    // int total = img->w*img->h*img->n;

    // printf("image w: %d, h: %d, n: %d\n",img->w, img->h, img->n);
    // printf("dst w: %d, h: %d\n",dst_w, dst_h);
    // printf("buffer w: %d, h: %d\n",buffer_width, buffer_height);

    for(int j = 0; j < dst_h;++j)
    {
        if (y + j >= buffer_height)
            continue;

        for(int i = 0; i < dst_w;++i)
        {
            if (x + i >= buffer_width)
                break;

            //printf("col: %d,row: %d\n",i,j);

            // int img_idx = (j*img->w+i)*img->n;
            // uint8_t r = img->data[img_idx];
            // uint8_t g = img->data[img_idx+1];
            // uint8_t b = img->data[img_idx+2];
            // uint8_t a = img->data[img_idx+3];


            int index = (j*dst_w + i);
            uint8_t r = *(img->data + (4*index) + 0);
            uint8_t g = *(img->data + (4*index) + 1);
            uint8_t b = *(img->data + (4*index) + 2);
            uint8_t a = *(img->data + (4*index) + 3);

            uint32_t color = gfx_rgb_to_color(r,g,b);
            gfx_draw_pixela(i+x, j+y, color, (float)a/255.0);

        }
    }

    return true;
}

void gfx_draw_rect(int x, int y, int w, int h, uint32_t color, bool filled)
{
    unsigned char* dst = (unsigned char*)back_buffer;
    dst = dst + (buffer_width*y) + x;

    if (dst < (unsigned char*)back_buffer) return;
    if (dst + (buffer_width*h)+w >= (unsigned char*)back_buffer + buffer_width*buffer_height) return;

    if(filled)
    {
        for(int i = 0; i < h; ++i)
        {
            memset(dst,color,w);
            dst += buffer_width;
        }
    }
    else
    {
        //top line
        memset(dst,color,w);
        dst += buffer_width;

        for(int i = 0; i < h-2; ++i)
        {
            *dst = color;
            dst+= (w-1);
            *dst = color;
            dst += (buffer_width - w + 1);
        }

        // bottom line
        memset(dst,color,w);
    }
}

void gfx_draw_pixela(int x, int y, uint32_t color, float alpha)
{
	// c is brightness. 0 <= c <= 1
	uint32_t* dst = back_buffer;
	dst = dst + (buffer_width*y) + x;

	if (dst < back_buffer)
		return;

	if (dst >= back_buffer + (buffer_width*buffer_height))
		return;

    if (x < 0 || x >= buffer_width)
        return;

    uint8_t* p = (uint8_t*)dst;

    uint8_t r0,g0,b0;
    uint8_t r1,g1,b1;

    gfx_color_to_rgb(*dst,&r0,&g0,&b0);
    gfx_color_to_rgb(color,&r1,&g1,&b1);
        
    uint8_t r = (alpha)*r1 + (1.0-alpha)*r0;
    uint8_t g = (alpha)*g1 + (1.0-alpha)*g0;
    uint8_t b = (alpha)*b1 + (1.0-alpha)*b0;
    
	*dst = gfx_rgb_to_color(r,g,b);
}

void gfx_draw_pixel(int x, int y, uint32_t color)
{
    gfx_draw_pixela(x,y,color, 1.0);
}

// THE EXTREMELY FAST LINE ALGORITHM Variation C (Addition)
void gfx_draw_line(int x, int y, int x2, int y2, uint32_t color)
{
	bool yLonger = false;
	int incrementVal, endVal;

	int shortLen = y2 - y;
	int longLen = x2 - x;
	if (abs(shortLen)>abs(longLen)) {
		int swap = shortLen;
		shortLen = longLen;
		longLen = swap;
		yLonger = true;
	}

	endVal = longLen;
	if (longLen<0) {
		incrementVal = -1;
		longLen = -longLen;
	}
	else incrementVal = 1;

	double decInc;
	if (longLen == 0) decInc = (double)shortLen;
	else decInc = ((double)shortLen / (double)longLen);
	double j = 0.0;
	if (yLonger) {
		for (int i = 0; i != endVal; i += incrementVal) {
			gfx_draw_pixel(x + (int)j, y + i, color);
			j += decInc;
		}
	}
	else {
		for (int i = 0; i != endVal; i += incrementVal) {
			gfx_draw_pixel(x + i, y + (int)j, color);
			j += decInc;
		}
	}
}

void gfx_draw_circle(int x0, int y0, int r, uint32_t color, bool filled)
{
	int x = r;
	int y = 0;
	int err = 0;

	while (x >= y)
	{
		if (filled)
		{
			gfx_draw_line(x0 + x, y0 + y, x0 - x, y0 + y, color);
			gfx_draw_line(x0 + y, y0 + x, x0 - y, y0 + x, color);
			gfx_draw_line(x0 - x, y0 - y, x0 + x, y0 - y, color);
			gfx_draw_line(x0 - y, y0 - x, x0 + y, y0 - x, color);
		}
		else
		{
			gfx_draw_pixel(x0 + x, y0 + y, color);
			gfx_draw_pixel(x0 + y, y0 + x, color);
			gfx_draw_pixel(x0 - y, y0 + x, color);
			gfx_draw_pixel(x0 - x, y0 + y, color);
			gfx_draw_pixel(x0 - x, y0 - y, color);
			gfx_draw_pixel(x0 - y, y0 - x, color);
			gfx_draw_pixel(x0 + y, y0 - x, color);
			gfx_draw_pixel(x0 + x, y0 - y, color);
		}

		if (err <= 0)
		{
			y += 1;
			err += 2 * y + 1;
		}
		if (err > 0)
		{
			x -= 1;
			err -= 2 * x + 1;
		}
	}
}

void gfx_draw_circle_wu(int radius)
{
}

void gfx_draw_ellipse(int origin_x,int origin_y, int w, int h, uint32_t color, bool filled)
{
	int hh = h * h;
	int ww = w * w;
	int hhww = hh * ww;
	int x0 = w;
	int dx = 0;

	// do the horizontal diameter
	if (filled)
		for (int x = -w; x <= w; x++)
			gfx_draw_pixel(origin_x + x, origin_y, color);
	else
	{
		gfx_draw_pixel(origin_x - w, origin_y, color);
		gfx_draw_pixel(origin_x + w, origin_y, color);
	}

	// now do both halves at the same time, away from the diameter
	for (int y = 1; y <= h; y++)
	{
		int x1 = x0 - (dx - 1);  // try slopes of dx - 1 or more
		for (; x1 > 0; x1--)
			if (x1*x1*hh + y*y*ww <= hhww)
				break;
		dx = x0 - x1;  // current approximation of the slope
		x0 = x1;

		if (filled)
		{
			for (int x = -x0; x <= x0; x++)
			{
				gfx_draw_pixel(origin_x + x, origin_y - y, color);
				gfx_draw_pixel(origin_x + x, origin_y + y, color);
			}
		}
		else
		{
			if (dx <= 0)
			{

				gfx_draw_pixel(origin_x - x0, origin_y - y, color);
				gfx_draw_pixel(origin_x - x0, origin_y + y, color);
				gfx_draw_pixel(origin_x + x0, origin_y - y, color);
				gfx_draw_pixel(origin_x + x0, origin_y + y, color);
			}
			else
			{
				for (int i = 0; i < dx; ++i)
				{
					gfx_draw_pixel(origin_x - x0 - i, origin_y - y, color);
					gfx_draw_pixel(origin_x - x0 - i, origin_y + y, color);
					gfx_draw_pixel(origin_x + x0 + i, origin_y - y, color);
					gfx_draw_pixel(origin_x + x0 + i, origin_y + y, color);
				}
			}

		}
	}
}

void gfx_color_to_rgb(uint32_t color, uint8_t* r,uint8_t* g,uint8_t* b)
{

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    *r = (color >>  0) & 0xFF;
    *g = (color >>  8) & 0xFF;
    *b = (color >>  16) & 0xFF;
#else
    *r = (color >>  24) & 0xFF;
    *g = (color >>  16) & 0xFF;
    *b = (color >>  8) & 0xFF;
#endif
}

uint32_t gfx_rgb_to_color(uint8_t r,uint8_t g,uint8_t b)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (r << 0 | g << 8 | b << 16);
#else
    return (r << 24 | g << 16 | b << 8);
#endif
}



#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GL/glew.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG 
#include "util/stb_image.h"

#include "common.h"
#include "shader.h"
#include "settings.h"
#include "gfx.h"

#define MAX_GFX_IMAGES 32

static GLuint texture = 0;
static GLuint vao, vbo, ibo;

static uint32_t* back_buffer;
static uint32_t buffer_width;
static uint32_t buffer_height;

void gfx_init(int width, int height,int border)
{
    printf("GL version: %s\n",glGetString(GL_VERSION));

    glClearColor(0.15f, 0.15f, 0.15f, 0.0f);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    float zx = 1.0 - (border / (float)window_width);
    float zy = 1.0 - (border / (float)window_height);

    Vertex vertices[4] = 
    {
        {{-zx, -zy},{+0.0,+0.0}},
        {{-zx, +zy},{+0.0,+1.0}},
        {{+zx, +zy},{+1.0,+1.0}},
        {{+zx, -zy},{+1.0,+0.0}},
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

static uint32_t get_img_pixel(GFXImage* img, int x, int y, float* alpha)
{
    int index = sizeof(uint32_t) * ((y*img->w) + x);

    uint8_t r = *(img->data + index + 0);
    uint8_t g = *(img->data + index + 1);
    uint8_t b = *(img->data + index + 2);
    uint8_t a = *(img->data + index + 3);

    *alpha = (a / 255.0);

    return gfx_rgb_to_color(r,g,b);
}

static void _draw_image()
{

}

bool gfx_draw_image(int img_index, int x, int y)
{
    if(img_index < 0)
    {
        printf("invalid image index!\n");
        return false;
    }

    GFXImage* img = &gfx_images[img_index];

    // int total = img->w*img->h*img->n;

    for(int j = 0; j < img->h;++j)
    {
        if (y + j >= buffer_height)
            continue;

        for(int i = 0; i < img->w;++i)
        {
            if (x + i >= buffer_width)
                break;
            
            float img_a = 1.0;
            uint32_t color = get_img_pixel(img, i, j, &img_a);

            //gfx_draw_pixela(i+x, j+y, color, img_a);
            gfx_draw_pixel(i+x, j+y, color);
        }
    }

    return true;
}

bool gfx_draw_image_scaled(int img_index, int x, int y,float scale, float alpha)
{
    if(img_index < 0)
    {
        printf("invalid image index!\n");
        return false;
    }

    GFXImage* img = &gfx_images[img_index];

    int new_width = (int)(img->w*scale);
    int new_height = (int)(img->h*scale);
    // int total = img->w*img->h*img->n;

    for(int j = 0; j < new_height;++j)
    {
        if (y + j >= buffer_height)
            continue;

        for(int i = 0; i < new_width;++i)
        {
            if (x + i >= buffer_width)
                break;
            
            int n_i = (int)round(i/scale);
            int n_j = (int)round(j/scale);

            float img_a = 1.0;
            uint32_t color = get_img_pixel(img, n_i, n_j, &img_a);
            gfx_draw_pixela(i+x, j+y, color, alpha*img_a);
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
	uint32_t* dst = back_buffer;
	dst = dst + (buffer_width*y) + x;

	if (dst < back_buffer)
		return;

	if (dst >= back_buffer + (buffer_width*buffer_height))
		return;

    if (x < 0 || x >= buffer_width)
        return;

    if(color > 0)
        *dst = color;
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

/*
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
*/

static void draw_circle_pixels(int x, int y, int i, int j, uint32_t color, float alpha)
{
    gfx_draw_pixela(x+i,y+j,color, alpha);
    gfx_draw_pixela(x+i,y-j,color, alpha);
    gfx_draw_pixela(x-i,y+j,color, alpha);
    gfx_draw_pixela(x-i,y-j,color, alpha);
    gfx_draw_pixela(x+j,y+i,color, alpha);
    gfx_draw_pixela(x+j,y-i,color, alpha);
    gfx_draw_pixela(x-j,y+i,color, alpha);
    gfx_draw_pixela(x-j,y-i,color, alpha);
}

void gfx_draw_circle(int x, int y, int outer_radius, uint32_t color)
{
    int i = 0;
    int j = outer_radius;

    double last_fade_amount = 0;
    double fade_amount = 0;

    const float MAX_OPAQUE = 100.0;

    while(i < j)
    {
        double height = sqrt(MAX(outer_radius * outer_radius - i * i, 0));
        fade_amount = MAX_OPAQUE * (ceil(height) - height);
        printf("fade_amount: %f\n",fade_amount);

        if(fade_amount < last_fade_amount)
            --j;

        // Opaqueness reset so drop down a row.
        last_fade_amount = fade_amount;

        // The API needs integers, so convert here now we've checked if 
        // it dropped.
        int fade_amount_i = (int)fade_amount;

        // We're fading out the current j row, and fading in the next one down.
        draw_circle_pixels(x,y,i,j,color,(MAX_OPAQUE - fade_amount_i) / 100.0);
        draw_circle_pixels(x,y,i,j-1,color,fade_amount_i/100.0);

        ++i;
    }
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



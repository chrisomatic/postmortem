#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <GL/glew.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "util/stb_image.h"

#include "shader.h"
#include "window.h"
#include "camera.h"
#include "log.h"
#include "gfx.h"

static GLuint quad_vao, quad_vbo;
static GLuint line_vao,line_vbo;

GFXImage gfx_images[MAX_GFX_IMAGES] = {0};

static Matrix proj_matrix;

static GLint loc_basic_image;
static GLint loc_basic_tint_color;
static GLint loc_basic_opacity;
static GLint loc_basic_model;
static GLint loc_basic_view;
static GLint loc_basic_proj;

static GLint loc_sprite_image;
static GLint loc_sprite_tint_color;
static GLint loc_sprite_opacity;
static GLint loc_sprite_model;
static GLint loc_sprite_view;
static GLint loc_sprite_proj;
static GLint loc_sprite_num_in_row;
static GLint loc_sprite_num_in_col;
static GLint loc_sprite_index;

static GLint loc_shape_color;
static GLint loc_shape_opacity;
static GLint loc_shape_model;
static GLint loc_shape_view;
static GLint loc_shape_proj;

static GLint loc_line_opacity;
static GLint loc_line_view;
static GLint loc_line_proj;



typedef struct
{
    int w,h,n;
    unsigned char* data;
    unsigned char* upright_data;
} _Image;

static bool load_image(const char* image_path, _Image* image);
static int assign_image(GFXSubImageData* sub_image_data, _Image* image);

static int image_find_first_visible_rowcol(int side, int img_w, int img_h, int img_n, unsigned char* data);


#define MAX_LINES 100

LinePoint line_points[2*MAX_LINES];
int num_line_points = 0;

void gfx_init(int width, int height)
{
    printf("GL version: %s\n",glGetString(GL_VERSION));

    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);

    Vertex quad[] =
    {
        {{-0.5, +0.5},{0.0,1.0}},
        {{+0.5, -0.5},{1.0,0.0}},
        {{-0.5, -0.5},{0.0,0.0}},
        {{-0.5, +0.5},{0.0,1.0}},
        {{+0.5, +0.5},{1.0,1.0}},
        {{+0.5, -0.5},{1.0,0.0}},
    };

    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vertex), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex), (const GLvoid*)8);

    // line
    glGenVertexArrays(1, &line_vao);
    glBindVertexArray(line_vao);

    glGenBuffers(1, &line_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_points), 0, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(LinePoint), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(LinePoint), (const GLvoid*)8);

    // shader locations
    loc_basic_image      = glGetUniformLocation(program_basic, "image");
    loc_basic_tint_color = glGetUniformLocation(program_basic, "tint_color");
    loc_basic_opacity    = glGetUniformLocation(program_basic, "opacity");
    loc_basic_model      = glGetUniformLocation(program_basic, "model");
    loc_basic_view       = glGetUniformLocation(program_basic, "view");
    loc_basic_proj       = glGetUniformLocation(program_basic, "projection");

    loc_sprite_image      = glGetUniformLocation(program_sprite, "image");
    loc_sprite_tint_color = glGetUniformLocation(program_sprite, "tint_color");
    loc_sprite_opacity    = glGetUniformLocation(program_sprite, "opacity");
    loc_sprite_model      = glGetUniformLocation(program_sprite, "model");
    loc_sprite_view       = glGetUniformLocation(program_sprite, "view");
    loc_sprite_proj       = glGetUniformLocation(program_sprite, "projection");
    loc_sprite_num_in_row = glGetUniformLocation(program_sprite, "num_sprites_in_row");
    loc_sprite_num_in_col = glGetUniformLocation(program_sprite, "num_sprites_in_col");
    loc_sprite_index      = glGetUniformLocation(program_sprite, "sprite_index");

    loc_shape_color      = glGetUniformLocation(program_shape, "color");
    loc_shape_opacity    = glGetUniformLocation(program_shape, "opacity");
    loc_shape_model      = glGetUniformLocation(program_shape, "model");
    loc_shape_view       = glGetUniformLocation(program_shape, "view");
    loc_shape_proj       = glGetUniformLocation(program_shape, "projection");

    loc_line_view        = glGetUniformLocation(program_line, "view");
    loc_line_proj        = glGetUniformLocation(program_line, "projection");
    loc_line_opacity     = glGetUniformLocation(program_line, "opacity");

    ortho(&proj_matrix,0.0,(float)width,(float)height,0.0, -1.0, 1.0);

    //print_matrix(&proj_matrix);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LINE_SMOOTH);
    glLineWidth(5.0);

    stbi_set_flip_vertically_on_load(1);

    for(int i = 0; i < MAX_GFX_IMAGES; ++i)
    {
        gfx_images[i].texture = -1;
    }
}

void gfx_clear_buffer(uint8_t r, uint8_t g, uint8_t b)
{
    glClearColor(r/255.0, g/255.0,b/255.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

static bool load_image(const char* image_path, _Image* image)
{
    image->data = stbi_load(image_path,&image->w,&image->h,&image->n,4);

    if(image->data != NULL)
    {
        printf("Loaded image: %s (x: %d, y: %d, n: %d)\n", image_path, image->w, image->h, image->n);
        image->upright_data = malloc(image->w*image->h*image->n*sizeof(image->data[0]));

        for(int _y = 0; _y < image->h; ++_y)
        {
            int y = image->h - _y - 1;
            for(int x = 0; x < image->w; ++x)
            {
                int index = (y*image->w + x)*image->n;
                int upright_index = (_y*image->w + x)*image->n;
                image->upright_data[upright_index+0] = image->data[index+0];
                image->upright_data[upright_index+1] = image->data[index+1];
                image->upright_data[upright_index+2] = image->data[index+2];
                image->upright_data[upright_index+3] = image->data[index+3];
            }
        }

        return true;
    }
    else
    {
        printf("Image data is NULL: %s\n", image_path);
        return false;
    }
}

static int assign_image(GFXSubImageData* sub_image_data, _Image* image)
{
    for(int i = 0; i < MAX_GFX_IMAGES; ++i)
    {
        if(gfx_images[i].texture == -1)
        {
            GFXImage* img = &gfx_images[i];

            img->w = image->w;
            img->h = image->h;
            img->n = image->n;

            printf("Assigning image to index: %d\n", i);

            if(sub_image_data != NULL)
            {
                img->sub_img_data = calloc(1,sizeof(GFXSubImageData));
                img->sub_img_data->element_count = sub_image_data->element_count;
                img->sub_img_data->element_width = sub_image_data->element_width;
                img->sub_img_data->element_height = sub_image_data->element_height;
                img->sub_img_data->visible_rects = calloc(img->sub_img_data->element_count, sizeof(Rect));
                for(int e = 0; e < img->sub_img_data->element_count; ++e)
                {
                    memcpy(&img->sub_img_data->visible_rects[e], &sub_image_data->visible_rects[e], sizeof(Rect));
                }

                printf("Image set: width: %d, height: %d, count: %d\n", img->sub_img_data->element_width, img->sub_img_data->element_height, img->sub_img_data->element_count);
            }
            else
            {
                gfx_get_image_visible_rect(img->w, img->h, img->n, image->upright_data, &img->visible_rect);
                printf("Visible Rectangle: x: %.0f, y: %.0f, w: %.0f, h: %.0f\n", img->visible_rect.x, img->visible_rect.y, img->visible_rect.w, img->visible_rect.h);
            }

            glGenTextures(1, &img->texture);
            glBindTexture(GL_TEXTURE_2D, img->texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindTexture(GL_TEXTURE_2D, 0);
            return i;
        }
    }

    return -1;
}

int gfx_load_image(const char* image_path)
{
    _Image image = {0};
    bool load = load_image(image_path, &image);
    if(!load) return -1;

    int idx = assign_image(NULL, &image);
    free(image.data);
    free(image.upright_data);

    return idx;
}

int gfx_load_image_set(const char* image_path, int element_width, int element_height)
{
    _Image image = {0};
    bool load = load_image(image_path, &image);
    if(!load) return -1;

    int num_in_row = (image.w / element_width);     //cols
    int num_in_col = (image.h / element_height);    //rows
    int element_count = num_in_row*num_in_col;

    GFXSubImageData sid = {0};
    sid.element_width = element_width;
    sid.element_height = element_height;
    sid.element_count = element_count;
    sid.visible_rects = malloc(sid.element_count * sizeof(Rect));

    int n = image.n;
    unsigned char* temp_data = malloc(element_width*element_height*n*sizeof(unsigned char));

    for(int i = 0; i < sid.element_count; ++i)
    {
        int start_x = (i % num_in_row) * element_width;
        int start_y = (i / num_in_row) * element_height;
        for(int y = 0; y < element_height; ++y)
        {
            for(int x = 0; x < element_width; ++x)
            {
                int index = ((start_y+y)*image.w + (start_x+x)) * n;
                int sub_index = (y*element_width + x) * n;
                temp_data[sub_index] = image.upright_data[index];
                temp_data[sub_index+1] = image.upright_data[index+1];
                temp_data[sub_index+2] = image.upright_data[index+2];
                temp_data[sub_index+3] = image.upright_data[index+3];
            }
        }
        gfx_get_image_visible_rect(element_width, element_height, 4, temp_data, &sid.visible_rects[i]);
    }

    int idx = assign_image(&sid, &image);

    free(temp_data);
    free(image.data);
    free(image.upright_data);
    free(sid.visible_rects);

    return idx;
}


void gfx_get_image_visible_rect(int img_w, int img_h, int img_n, unsigned char* img_data, Rect* ret)
{
    int top = image_find_first_visible_rowcol(0, img_w, img_h, img_n, img_data);
    int bottom = image_find_first_visible_rowcol(2, img_w, img_h, img_n, img_data);
    int left = image_find_first_visible_rowcol(1, img_w, img_h, img_n, img_data);
    int right = image_find_first_visible_rowcol(3, img_w, img_h, img_n, img_data);

    int height = bottom - top + 1;  //top left is origin
    int width = right - left + 1;

    ret->w = (float)width;
    ret->h = (float)height;
    ret->x = (float)left;
    ret->y = (float)top;
}

void gfx_free_image(int img_index)
{
    if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
    {
        printf("%s: Invalid image index!\n", __func__);
        return;
    }
    GFXImage* img = &gfx_images[img_index];

    GFXSubImageData* sid = img->sub_img_data;
    if(sid != NULL)
    {
        if(sid->visible_rects != NULL) free(sid->visible_rects);
        free(sid);
    }

    //stbi_image_free(img->data);
    memset(img, 0, sizeof(GFXImage));
    img->texture = -1;
}

GFXImage* gfx_get_image_data(int img_index)
{
    if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
    {
        printf("%s: Invalid image index!\n", __func__);
        return NULL;
    }
    return &gfx_images[img_index];
}

void gfx_draw_rect_xywh(float x, float y, float w, float h, uint32_t color, float scale, float opacity)
{
    glUseProgram(program_shape);

    Matrix model = {0};

    Vector3f pos = {x+w/2.0,y+h/2.0,0.0};
    Vector3f rot = {0.0,0.0,0.0};
    Vector3f sca = {scale*w,-scale*h,1.0};

    get_model_transform(&pos,&rot,&sca,&model);
    Matrix* view = get_camera_transform();

    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color >> 0;

    glUniform3f(loc_shape_color,r/255.0,g/255.0,b/255.0);
    glUniform1f(loc_shape_opacity,opacity);

    glUniformMatrix4fv(loc_shape_model,1,GL_TRUE,&model.m[0][0]);
    glUniformMatrix4fv(loc_shape_view,1,GL_TRUE,&view->m[0][0]);
    glUniformMatrix4fv(loc_shape_proj,1,GL_TRUE,&proj_matrix.m[0][0]);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBindVertexArray(quad_vao);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_TRIANGLES,0,6);//,GL_UNSIGNED_INT,0);

    glDisableVertexAttribArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);
}

void gfx_draw_rect(Rect* r, uint32_t color, float scale, float opacity)
{
    gfx_draw_rect_xywh(r->x, r->y, r->w, r->h, color, scale, opacity);
}

bool gfx_draw_image(int img_index, float x, float y, uint32_t color, float scale, float rotation, float opacity)
{
    if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
    {
        printf("%s: Invalid image index!\n", __func__);
        return false;
    }

    GFXImage* img = &gfx_images[img_index];

    // int total = img->w*img->h*img->n;
    glUseProgram(program_basic);

    Matrix model = {0};

    Vector3f pos = {x+img->w/2.0,y+img->h/2.0,0.0};
    Vector3f rot = {0.0,0.0,360.0-rotation};
    Vector3f sca = {scale*img->w,-scale*img->h,1.0};

    get_model_transform(&pos,&rot,&sca,&model);
    Matrix* view = get_camera_transform();

    //print_matrix(&model);
    //print_matrix(view);
    //print_matrix(&proj_matrix);

    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color >> 0;

    glUniform3f(loc_basic_tint_color,r/255.0,g/255.0,b/255.0);
    glUniform1f(loc_basic_opacity,opacity);

    glUniformMatrix4fv(loc_basic_model,1,GL_TRUE,&model.m[0][0]);
    glUniformMatrix4fv(loc_basic_view,1,GL_TRUE,&view->m[0][0]);
    glUniformMatrix4fv(loc_basic_proj,1,GL_TRUE,&proj_matrix.m[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, img->texture);

    glUniform1i(loc_basic_image, 0);

    glBindVertexArray(quad_vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES,0,6);//,GL_UNSIGNED_INT,0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);

    return true;
}

bool gfx_draw_sub_image(int img_index, int sprite_index, float x, float y, uint32_t color, float scale, float rotation, float opacity)
{
    if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
    {
        printf("%s: Invalid image index!\n", __func__);
        return false;
    }

    GFXImage* img = &gfx_images[img_index];
    GFXSubImageData* sid = img->sub_img_data;

    if(sid == NULL)
    {
        printf("Not a sub image set (%d)\n", img_index);
        return false;
    }

    if(sprite_index >= sid->element_count)
    {
        printf("Invalid sprite index: %d (%d, %d)\n", sprite_index, img_index, sid->element_count);
        return false;
    }

    glUseProgram(program_sprite);

    Matrix model = {0};

    Vector3f pos = {x+sid->element_width/2.0,y+sid->element_height/2.0,0.0};
    Vector3f rot = {0.0,0.0,360.0-rotation};
    Vector3f sca = {scale*sid->element_width,-scale*sid->element_height,1.0};

    get_model_transform(&pos,&rot,&sca,&model);
    Matrix* view = get_camera_transform();

    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color >> 0;

    int num_in_row = (img->w / sid->element_width);
    int num_in_col = (img->h / sid->element_height);
    //printf("num_in_row: %d, sprite_index: %d\n",num_in_row, sprite_index);

    glUniform3f(loc_sprite_tint_color,r/255.0,g/255.0,b/255.0);
    glUniform1f(loc_sprite_opacity,opacity);
    glUniform1i(loc_sprite_num_in_row,num_in_row);
    glUniform1i(loc_sprite_num_in_col,num_in_col);
    glUniform1i(loc_sprite_index,sprite_index);

    glUniformMatrix4fv(loc_sprite_model,1,GL_TRUE,&model.m[0][0]);
    glUniformMatrix4fv(loc_sprite_view,1,GL_TRUE,&view->m[0][0]);
    glUniformMatrix4fv(loc_sprite_proj,1,GL_TRUE,&proj_matrix.m[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, img->texture);

    glUniform1i(loc_sprite_image, 0);

    glBindVertexArray(quad_vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES,0,6);//,GL_UNSIGNED_INT,0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);

    return true;

}

void gfx_clear_lines()
{
    num_line_points = 0;
}

void gfx_add_line(float x0, float y0, float x1, float y1, uint32_t color)
{
    if(num_line_points >= 2*MAX_LINES-1)
    {
        LOGW("Too many lines, can't add any more!");
        return;
    }

    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color >> 0;

    float rf = (float)r/255.0;
    float gf = (float)g/255.0;
    float bf = (float)b/255.0;

    LinePoint points[2] = {
        {{x0,y0},{rf,gf,bf}},
        {{x1,y1},{rf,gf,bf}}
    };

    memcpy(&line_points[num_line_points],points,sizeof(points));
    num_line_points += 2;
}

void gfx_draw_lines()
{
    Matrix* view = get_camera_transform();

    glUseProgram(program_line);

    glUniformMatrix4fv(loc_line_view,1,GL_TRUE,&view->m[0][0]);
    glUniformMatrix4fv(loc_line_proj,1,GL_TRUE,&proj_matrix.m[0][0]);

    glUniform1f(loc_line_opacity,1.0);

    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_line_points*sizeof(LinePoint), line_points);

    glBindVertexArray(line_vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_LINES,0,num_line_points);//,GL_UNSIGNED_INT,0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glUseProgram(0);

}


// get first row or col that's not empty
// side: 0=top,1=left,2=bottom,3=right
static int image_find_first_visible_rowcol(int side, int img_w, int img_h, int img_n, unsigned char* data)
{
    if(side == 0 || side == 2)
    {
        bool prior_empty = true;
        for(int _y = 0; _y < img_h; ++_y)
        {

            int y = _y;
            // change scan direction
            if(side == 2)
                y = img_h - _y - 1;

            bool row_empty = true;

            for(int x = 0; x < img_w; ++x)
            {
                int index = (y*img_w + x);
                uint8_t r = *(data + (img_n*index) + 0);
                uint8_t g = *(data + (img_n*index) + 1);
                uint8_t b = *(data + (img_n*index) + 2);
                uint8_t a = *(data + (img_n*index) + 3);
                if(a != 0 && !(r == 0xFF && g == 0 &&  b == 0xFF))
                {
                    row_empty = false;
                    break;
                }
            }

            if(prior_empty && !row_empty)
            {
                return y;
            }
            prior_empty = row_empty;
        }

        if(side == 2) return img_h-1;  //bottom
        return 0;   //top
    }
    else if(side == 1 || side == 3)
    {
        bool prior_empty = true;
        for(int _x = 0; _x < img_w; ++_x)
        {
            int x = _x;
            if(side == 3)
                x = img_w-_x-1;

            bool col_empty = true;

            for(int y = 0; y < img_h; ++y)
            {

                int index = (y*img_w + x);
                uint8_t r = *(data + (img_n*index) + 0);
                uint8_t g = *(data + (img_n*index) + 1);
                uint8_t b = *(data + (img_n*index) + 2);
                uint8_t a = *(data + (img_n*index) + 3);
                if(a != 0 && !(r == 0xFF && g == 0 &&  b == 0xFF))
                {
                    col_empty = false;
                    break;
                }
            }

            if(prior_empty && !col_empty)
            {
                return x;
            }
            prior_empty = col_empty;
        }

        if(side == 3) return img_w-1;
        return 0;
    }
    return -1;
}

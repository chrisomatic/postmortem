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
static GLuint font_vao, font_vbo;
static GLuint line_vao,line_vbo;

// GFXImage gfx_images[MAX_GFX_IMAGES] = {0};

static Matrix proj_matrix;

static GLint loc_basic_image;
static GLint loc_basic_tint_color;
static GLint loc_basic_opacity;
static GLint loc_basic_brightness;
static GLint loc_basic_model;
static GLint loc_basic_view;
static GLint loc_basic_proj;

static GLint loc_sprite_image;
static GLint loc_sprite_tint_color;
static GLint loc_sprite_opacity;
static GLint loc_sprite_brightness;
static GLint loc_sprite_model;
static GLint loc_sprite_view;
static GLint loc_sprite_proj;

static GLint loc_shape_color;
static GLint loc_shape_opacity;
static GLint loc_shape_model;
static GLint loc_shape_view;
static GLint loc_shape_proj;

static GLint loc_line_opacity;
static GLint loc_line_view;
static GLint loc_line_proj;

static GLint loc_font_image;
static GLint loc_font_fg_color;
static GLint loc_font_bg_color;
static GLint loc_font_tex;
static GLint loc_font_px_range;
static GLint loc_font_opacity;
static GLint loc_font_model;
static GLint loc_font_view;
static GLint loc_font_proj;

#define DEBUG_PRINT()   printf("%d %s %s()\n", __LINE__, __FILE__, __func__)


static int assign_image(GFXSubImageData* sub_image_data, GFXImageData* image, bool linear_filter);

static int image_find_first_visible_rowcol(int side, int img_w, int img_h, int img_n, unsigned char* data);

#define MAX_LINES 100

LinePoint line_points[2*MAX_LINES];
int num_line_points = 0;

typedef struct
{
    float l,b,r,t;
} CharBox;

typedef struct
{
    float advance;
    CharBox plane_box;
    CharBox pixel_box;
    CharBox tex_coords;
    float w,h;
} FontChar;

int font_image;
FontChar font_chars[255];

static void load_font()
{
    font_image = gfx_load_image2("core/fonts/atlas.png", false, true, 0, 0, NULL);
    LOGI("Font image index: %d",font_image);

    FILE* fp = fopen("core/fonts/atlas_layout.csv","r");

    if(!fp)
    {
        LOGW("Failed to load font layout file");
        return;
    }

    char line[256] = {0};

    while(fgets(line,255,fp) != NULL)
    {
        int char_index = 0;
        float advance, pl_l, pl_b, pl_r, pl_t,  px_l, px_b, px_r, px_t;

        int n = sscanf(line,"%d,%f,%f,%f,%f,%f,%f,%f,%f,%f ",&char_index,&advance,&pl_l, &pl_b, &pl_r, &pl_t, &px_l, &px_b, &px_r, &px_t);

        if(n != 10)
            continue;

        font_chars[char_index].advance = advance;

        font_chars[char_index].plane_box.l = pl_l;
        font_chars[char_index].plane_box.b = pl_b;
        font_chars[char_index].plane_box.r = pl_r;
        font_chars[char_index].plane_box.t = pl_t;

        font_chars[char_index].pixel_box.l = px_l;
        font_chars[char_index].pixel_box.b = px_b;
        font_chars[char_index].pixel_box.r = px_r;
        font_chars[char_index].pixel_box.t = px_t;

        font_chars[char_index].tex_coords.l = px_l/404.0;
        font_chars[char_index].tex_coords.b = 1.0 - (px_b/404.0);
        font_chars[char_index].tex_coords.r = px_r/404.0;
        font_chars[char_index].tex_coords.t = 1.0 - (px_t/404.0);

        font_chars[char_index].w = px_r - px_l;
        font_chars[char_index].h = px_b - px_t;

        // printf("font_char %d: %f [%f %f %f %f], [%f %f %f %f], [%f %f %f %f]\n",char_index,advance,pl_l,pl_b,pl_r,pl_t,px_l,px_b,px_r,px_t,px_l/512.0,px_b/512.0,px_r/512.0,px_t/512.0);
    }

    fclose(fp);
}


void gfx_image_init()
{
    stbi_set_flip_vertically_on_load(0);

    for(int i = 0; i < MAX_GFX_IMAGES; ++i)
    {
        // gfx_images[i].texture = -1;
        gfx_images2[i].texture = -1;
    }
}

void gfx_init(int width, int height)
{
    LOGI("GL version: %s",glGetString(GL_VERSION));

    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);

    // quad
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

    // font char
    glGenVertexArrays(1, &font_vao);
    glBindVertexArray(font_vao);

    glGenBuffers(1, &font_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, font_vbo);
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
    loc_basic_brightness = glGetUniformLocation(program_basic, "brightness");
    loc_basic_model      = glGetUniformLocation(program_basic, "model");
    loc_basic_view       = glGetUniformLocation(program_basic, "view");
    loc_basic_proj       = glGetUniformLocation(program_basic, "projection");

    loc_sprite_image      = glGetUniformLocation(program_sprite, "image");
    loc_sprite_tint_color = glGetUniformLocation(program_sprite, "tint_color");
    loc_sprite_opacity    = glGetUniformLocation(program_sprite, "opacity");
    loc_sprite_brightness = glGetUniformLocation(program_sprite, "brightness");
    loc_sprite_model      = glGetUniformLocation(program_sprite, "model");
    loc_sprite_view       = glGetUniformLocation(program_sprite, "view");
    loc_sprite_proj       = glGetUniformLocation(program_sprite, "projection");

    loc_shape_color      = glGetUniformLocation(program_shape, "color");
    loc_shape_opacity    = glGetUniformLocation(program_shape, "opacity");
    loc_shape_model      = glGetUniformLocation(program_shape, "model");
    loc_shape_view       = glGetUniformLocation(program_shape, "view");
    loc_shape_proj       = glGetUniformLocation(program_shape, "projection");

    loc_line_view        = glGetUniformLocation(program_line, "view");
    loc_line_proj        = glGetUniformLocation(program_line, "projection");
    loc_line_opacity     = glGetUniformLocation(program_line, "opacity");

    loc_font_image    = glGetUniformLocation(program_font, "image");
    loc_font_fg_color = glGetUniformLocation(program_font, "fg_color");
    loc_font_bg_color = glGetUniformLocation(program_font, "bg_color");
    loc_font_px_range = glGetUniformLocation(program_font, "px_range");
    loc_font_tex      = glGetUniformLocation(program_font, "tex");
    loc_font_model    = glGetUniformLocation(program_font, "model");
    loc_font_view     = glGetUniformLocation(program_font, "view");
    loc_font_proj     = glGetUniformLocation(program_font, "projection");

    ortho(&proj_matrix,0.0,(float)width,(float)height,0.0, -1.0, 1.0);

    //print_matrix(&proj_matrix);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LINE_SMOOTH);
    glLineWidth(5.0);

    gfx_image_init();

    load_font();
}

void gfx_clear_buffer(uint8_t r, uint8_t g, uint8_t b)
{
    glClearColor(r/255.0, g/255.0,b/255.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

// static int assign_image(GFXSubImageData* sub_image_data, GFXImageData* image, bool linear_filter)
// {
//     for(int i = 0; i < MAX_GFX_IMAGES; ++i)
//     {
//         if(gfx_images[i].texture == -1)
//         {
//             GFXImage* img = &gfx_images[i];

//             img->w = image->w;
//             img->h = image->h;
//             img->n = image->n;

//             LOGI("Assigning image to index: %d", i);

//             if(sub_image_data != NULL)
//             {
//                 img->sub_img_data = calloc(1,sizeof(GFXSubImageData));
//                 img->sub_img_data->element_count = sub_image_data->element_count;
//                 img->sub_img_data->elements_per_row = sub_image_data->elements_per_row;
//                 img->sub_img_data->elements_per_col = sub_image_data->elements_per_col;
//                 img->sub_img_data->element_width = sub_image_data->element_width;
//                 img->sub_img_data->element_height = sub_image_data->element_height;
//                 img->sub_img_data->visible_rects = calloc(img->sub_img_data->element_count, sizeof(Rect));
//                 img->sub_img_data->sprite_rects = calloc(img->sub_img_data->element_count, sizeof(Rect));
//                 for(int e = 0; e < img->sub_img_data->element_count; ++e)
//                 {
//                     memcpy(&img->sub_img_data->visible_rects[e], &sub_image_data->visible_rects[e], sizeof(Rect));
//                     memcpy(&img->sub_img_data->sprite_rects[e], &sub_image_data->sprite_rects[e], sizeof(Rect));
//                 }
//                 LOGI("Image set: width: %d, height: %d, count: %d", img->sub_img_data->element_width, img->sub_img_data->element_height, img->sub_img_data->element_count);
//             }
//             else
//             {
//                 gfx_get_image_visible_rect(img->w, img->h, img->n, image->data, &img->visible_rect);
//                 img->sprite_rect.w = img->visible_rect.w / img->w;
//                 img->sprite_rect.h = img->visible_rect.h / img->h;
//                 img->sprite_rect.x = (img->visible_rect.x) / img->w;
//                 img->sprite_rect.y = (img->visible_rect.y) / img->h;
//                 LOGI("Visible Rectangle: x: %.0f, y: %.0f, w: %.0f, h: %.0f", img->visible_rect.x, img->visible_rect.y, img->visible_rect.w, img->visible_rect.h);
//             }

//             glGenTextures(1, &img->texture);
//             glBindTexture(GL_TEXTURE_2D, img->texture);
//             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data);

//             if(linear_filter)
//             {
//                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//             }
//             else
//             {
//                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//             }

//             glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//             glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

//             glBindTexture(GL_TEXTURE_2D, 0);

//             if(img->texture == -1)
//                 img->texture = i; // @HACK

//             return i;
//         }
//     }

//     return -1;
// }

bool gfx_load_image_data(const char* image_path, GFXImageData* image, bool flip)
{
    LOGI("Loading image: %s",image_path);

    stbi_set_flip_vertically_on_load(flip);
    image->data = stbi_load(image_path,&image->w,&image->h,&image->n,4);

    if(image->data != NULL)
    {
        LOGI("Loaded image: %s (w: %d, h: %d, n: %d)", image_path, image->w, image->h, image->n);
        return true;
    }
    else
    {
        LOGE("Image data is NULL: %s", image_path);
        return false;
    }
}

// int gfx_load_image(const char* image_path, bool flip, bool linear_filter)
// {
//     GFXImageData image = {0};
//     bool load = gfx_load_image_data(image_path, &image, flip);
//     if(!load) return -1;

//     int idx = assign_image(NULL, &image, linear_filter);

//     stbi_image_free(image.data);

//     return idx;
// }

// int gfx_load_image_set(const char* image_path, int element_width, int element_height, GFXImageData* data)
// {
//     GFXImageData image = {0};
//     bool load = gfx_load_image_data(image_path, &image, false);
//     if(!load) return -1;

//     int num_cols = (image.w / element_width);     //cols
//     int num_rows = (image.h / element_height);    //rows
//     int element_count = num_cols*num_rows;

//     GFXSubImageData sid = {0};
//     sid.element_width = element_width;
//     sid.element_height = element_height;
//     sid.element_count = element_count;
//     sid.elements_per_row = num_cols;
//     sid.elements_per_col = num_rows;
//     sid.visible_rects = malloc(sid.element_count * sizeof(Rect));
//     sid.sprite_rects = malloc(sid.element_count * sizeof(Rect));

//     int n = image.n;
//     unsigned char* temp_data = malloc(element_width*element_height*n*sizeof(unsigned char));

//     for(int i = 0; i < sid.element_count; ++i)
//     {
//         int start_x = (i % num_cols) * element_width;
//         int start_y = (i / num_cols) * element_height;
//         for(int y = 0; y < element_height; ++y)
//         {
//             for(int x = 0; x < element_width; ++x)
//             {
//                 int index = ((start_y+y)*image.w + (start_x+x)) * n;
//                 int sub_index = (y*element_width + x) * n;
//                 for(int _n = 0; _n < n; ++_n)
//                     temp_data[sub_index+_n] = image.data[index+_n];
//             }
//         }

//         gfx_get_image_visible_rect(element_width, element_height, n, temp_data, &sid.visible_rects[i]);

//         Rect* vr = &sid.visible_rects[i];
//         sid.sprite_rects[i].x = (float)(start_x+vr->x) / image.w;
//         sid.sprite_rects[i].y = (float)(start_y+vr->y) / image.h;
//         sid.sprite_rects[i].w = vr->w / image.w;
//         sid.sprite_rects[i].h = vr->h / image.h;
//     }

//     int idx = assign_image(&sid, &image, false);

//     free(temp_data);
//     free(sid.visible_rects);
//     free(sid.sprite_rects);

//     if(data != NULL)
//     {
//         memcpy(data, &image, sizeof(GFXImageData));
//         int num_bytes = image.w*image.h*image.n*sizeof(unsigned char);
//         data->data = malloc(num_bytes);
//         memcpy(data->data, image.data, num_bytes);
//     }

//     stbi_image_free(image.data);

//     return idx;
// }


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
    ret->x = (float)left + ret->w/2.0;
    ret->y = (float)top + ret->h/2.0;
}

// void gfx_free_image(int img_index)
// {
//     if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
//     {
//         LOGE("%s: Invalid image index!", __func__);
//         return;
//     }
//     GFXImage* img = &gfx_images[img_index];

//     GFXSubImageData* sid = img->sub_img_data;
//     if(sid != NULL)
//     {
//         if(sid->visible_rects != NULL) free(sid->visible_rects);
//         if(sid->sprite_rects != NULL) free(sid->sprite_rects);
//         free(sid);
//     }

//     memset(img, 0, sizeof(GFXImage));
//     img->texture = -1;
// }

GFXImage2* gfx_get_image_data(int img_index)
{
    if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
    {
        LOGE("%s: Invalid image index!", __func__);
        return NULL;
    }
    return &gfx_images2[img_index];
}

void gfx_draw_rect_xywh(float x, float y, float w, float h, uint32_t color, float scale, float opacity, bool filled, bool in_world)
{
    glUseProgram(program_shape);

    Matrix model = {0};

    Vector3f pos = {x,y,0.0};
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

    if(in_world)
        glUniformMatrix4fv(loc_shape_view,1,GL_TRUE,&view->m[0][0]);
    else
        glUniformMatrix4fv(loc_shape_view,1,GL_TRUE,&IDENTITY_MATRIX.m[0][0]);

    glUniformMatrix4fv(loc_shape_proj,1,GL_TRUE,&proj_matrix.m[0][0]);

    if(filled)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glBindVertexArray(quad_vao);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_TRIANGLES,0,6);//,GL_UNSIGNED_INT,0);

    glDisableVertexAttribArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);
}

void gfx_draw_rect(Rect* r, uint32_t color, float scale, float opacity, bool filled, bool in_world)
{
    gfx_draw_rect_xywh(r->x, r->y, r->w, r->h, color, scale, opacity, filled, in_world);
}

// bool gfx_draw_image(int img_index, float x, float y, uint32_t color, float scale, float rotation, float opacity)
// {
//     if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
//     {
//         LOGE("%s: Invalid image index!", __func__);
//         return false;
//     }

//     GFXImage* img = &gfx_images[img_index];

//     // int total = img->w*img->h*img->n;
//     glUseProgram(program_basic);

//     Matrix model = {0};

//     float vw = img->visible_rect.w;
//     float vh = img->visible_rect.h;

//     Rect* sr = &img->sprite_rect;
//     float sprite_x = sr->x-sr->w/2.0;
//     float sprite_y = sr->y-sr->h/2.0;
//     float sprite_w = sr->w;
//     float sprite_h = sr->h;

//     Vertex quad[] =
//     {
//         {{-0.5, +0.5},{sprite_x,sprite_y+sprite_h}},
//         {{+0.5, -0.5},{sprite_x+sprite_w,sprite_y}},
//         {{-0.5, -0.5},{sprite_x,sprite_y}},
//         {{-0.5, +0.5},{sprite_x,sprite_y+sprite_h}},
//         {{+0.5, +0.5},{sprite_x+sprite_w,sprite_y+sprite_h}},
//         {{+0.5, -0.5},{sprite_x+sprite_w,sprite_y}},
//     };

//     glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
//     glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);

//     Vector3f pos = {x,y,0.0};
//     Vector3f rot = {0.0,0.0,360.0-rotation};
//     Vector3f sca = {scale*vw,scale*vh,1.0};

//     get_model_transform(&pos,&rot,&sca,&model);
//     Matrix* view = get_camera_transform();

//     //print_matrix(&model);
//     //print_matrix(view);
//     //print_matrix(&proj_matrix);

//     uint8_t r = color >> 16;
//     uint8_t g = color >> 8;
//     uint8_t b = color >> 0;

//     glUniform3f(loc_basic_tint_color,r/255.0,g/255.0,b/255.0);
//     glUniform1f(loc_basic_opacity,opacity);
//     glUniform1f(loc_basic_brightness,1.0);

//     glUniformMatrix4fv(loc_basic_model,1,GL_TRUE,&model.m[0][0]);
//     glUniformMatrix4fv(loc_basic_view,1,GL_TRUE,&view->m[0][0]);
//     glUniformMatrix4fv(loc_basic_proj,1,GL_TRUE,&proj_matrix.m[0][0]);

//     glActiveTexture(GL_TEXTURE0);
//     glBindTexture(GL_TEXTURE_2D, img->texture);

//     glUniform1i(loc_basic_image, 0);

//     glBindVertexArray(quad_vao);
//     glEnableVertexAttribArray(0);
//     glEnableVertexAttribArray(1);

//     glDrawArrays(GL_TRIANGLES,0,6);//,GL_UNSIGNED_INT,0);

//     glDisableVertexAttribArray(0);
//     glDisableVertexAttribArray(1);

//     glBindVertexArray(0);
//     glBindTexture(GL_TEXTURE_2D,0);
//     glUseProgram(0);

//     return true;
// }

// bool gfx_draw_sub_image(int img_index, int sprite_index, float x, float y, uint32_t color, float scale, float rotation, float opacity)
// {
//     if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
//     {
//         LOGE("%s: Invalid image index!", __func__);
//         return false;
//     }

//     GFXImage* img = &gfx_images[img_index];
//     GFXSubImageData* sid = img->sub_img_data;

//     if(sid == NULL)
//     {
//         LOGE("Not a sub image set (%d)", img_index);
//         return false;
//     }

//     if(sprite_index >= sid->element_count)
//     {
//         LOGE("Invalid sprite index: %d (%d, %d)", sprite_index, img_index, sid->element_count);
//         return false;
//     }
//     float vw = sid->visible_rects[sprite_index].w;
//     float vh = sid->visible_rects[sprite_index].h;


//     glUseProgram(program_sprite);
//     Rect* sr = &sid->sprite_rects[sprite_index];
//     float sprite_x = sr->x-sr->w/2.0;
//     float sprite_y = sr->y-sr->h/2.0;
//     float sprite_w = sr->w;
//     float sprite_h = sr->h;

//     Vertex quad[] =
//     {
//         {{-0.5, +0.5},{sprite_x,sprite_y+sprite_h}},
//         {{+0.5, -0.5},{sprite_x+sprite_w,sprite_y}},
//         {{-0.5, -0.5},{sprite_x,sprite_y}},
//         {{-0.5, +0.5},{sprite_x,sprite_y+sprite_h}},
//         {{+0.5, +0.5},{sprite_x+sprite_w,sprite_y+sprite_h}},
//         {{+0.5, -0.5},{sprite_x+sprite_w,sprite_y}},
//     };

//     glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
//     glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);

//     Matrix model = {0};

//     Vector3f pos = {x,y,0.0};
//     Vector3f rot = {0.0,0.0,360.0-rotation};
//     Vector3f sca = {scale*vw,scale*vh,1.0};

//     get_model_transform(&pos,&rot,&sca,&model);
//     Matrix* view = get_camera_transform();

//     uint8_t r = color >> 16;
//     uint8_t g = color >> 8;
//     uint8_t b = color >> 0;

//     //printf("num_in_row: %d, sprite_index: %d\n",num_in_row, sprite_index);

//     glUniform3f(loc_sprite_tint_color,r/255.0,g/255.0,b/255.0);
//     glUniform1f(loc_sprite_opacity,opacity);
//     glUniform1f(loc_sprite_brightness,1.0);

//     glUniformMatrix4fv(loc_sprite_model,1,GL_TRUE,&model.m[0][0]);
//     glUniformMatrix4fv(loc_sprite_view,1,GL_TRUE,&view->m[0][0]);
//     glUniformMatrix4fv(loc_sprite_proj,1,GL_TRUE,&proj_matrix.m[0][0]);

//     glActiveTexture(GL_TEXTURE0);
//     glBindTexture(GL_TEXTURE_2D, img->texture);

//     glUniform1i(loc_sprite_image, 0);

//     glBindVertexArray(quad_vao);
//     glEnableVertexAttribArray(0);
//     glEnableVertexAttribArray(1);

//     glDrawArrays(GL_TRIANGLES,0,6);//,GL_UNSIGNED_INT,0);

//     glDisableVertexAttribArray(0);
//     glDisableVertexAttribArray(1);

//     glBindVertexArray(0);
//     glBindTexture(GL_TEXTURE_2D,0);
//     glUseProgram(0);

//     return true;

// }

// w,h
Vector2f gfx_string_get_size(float scale, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char str[256] = {0};
    vsprintf(str,fmt, args);
    va_end(args);

    float x_pos = 0.0;
    float fontsize = 64.0 * scale;

    char* c = str;

    for(;;)
    {
        if(*c == '\0')
            break;

        FontChar* fc = &font_chars[*c];

        x_pos += (fontsize*fc->advance);
        c++;
    }

    Vector2f ret = {x_pos, fontsize};
    return ret;
}

void gfx_anim_update(GFXAnimation* anim, double delta_t)
{
    if(anim->finite && anim->curr_loop >= anim->max_loops)
        return;
   
    anim->curr_frame_time += delta_t;
    if(anim->curr_frame_time >= anim->max_frame_time)
    {
        anim->curr_frame_time -= anim->max_frame_time;
        anim->curr_frame++;

        if(anim->curr_frame >= anim->max_frames)
        {
            anim->curr_frame = 0;

            if(anim->finite)
                anim->curr_loop++;
        }
    }
}

Vector2f gfx_draw_string(float x, float y, uint32_t color, float scale, float rotation, float opacity, bool in_world, bool drop_shadow, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char str[256] = {0};
    vsprintf(str,fmt, args);
    va_end(args);

    glUseProgram(program_font);

    Matrix* view = get_camera_transform();

    GFXImage2* img = &gfx_images2[font_image];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, img->texture);
    glUniform1i(loc_font_image, 0);

    glBindVertexArray(font_vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color >> 0;

    glUniform4f(loc_font_bg_color,0.0,0.0,0.0,1.0);
    glUniform1f(loc_font_px_range,4.0);

    char* c = str;

    float fontsize = 64.0 * scale;

    float x_pos = x;
    float y_pos = y+fontsize;

    for(;;)
    {
        if(*c == '\0')
            break;

        FontChar* fc = &font_chars[*c];

        float x0 = x_pos + fontsize*fc->plane_box.l;
        float y0 = y_pos - fontsize*fc->plane_box.t;
        float x1 = x_pos + fontsize*fc->plane_box.r;
        float y1 = y_pos - fontsize*fc->plane_box.b;

        /*
        printf("====== %c ========\n",*c);
        printf("x0: %f, y0: %f, x1: %f, y1: %f\n",x0,y0,x1,y1);
        printf("w: %f, h: %f\n",w,h);
        printf("advance: %f, advance px: %f\n",fc->advance, fontsize*fc->advance);
        printf("==================\n");
        */

        Vertex fontchar[] =
        {
            {{x0, y1},{fc->tex_coords.l,fc->tex_coords.b}},
            {{x1, y0},{fc->tex_coords.r,fc->tex_coords.t}},
            {{x0, y0},{fc->tex_coords.l,fc->tex_coords.t}},
            {{x0, y1},{fc->tex_coords.l,fc->tex_coords.b}},
            {{x1, y1},{fc->tex_coords.r,fc->tex_coords.b}},
            {{x1, y0},{fc->tex_coords.r,fc->tex_coords.t}},
        };
        
        glBindBuffer(GL_ARRAY_BUFFER, font_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fontchar), fontchar);

        Matrix model = {0};

        Vector3f pos = {0.0,0.0,0.0};
        Vector3f rot = {0.0,0.0,rotation};
        Vector3f sca = {1.0,1.0,1.0};
        get_model_transform(&pos,&rot,&sca,&model);

        if(in_world)
            glUniformMatrix4fv(loc_font_view,1,GL_TRUE,&view->m[0][0]);
        else
            glUniformMatrix4fv(loc_font_view,1,GL_TRUE,&IDENTITY_MATRIX.m[0][0]);

        glUniformMatrix4fv(loc_font_proj,1,GL_TRUE,&proj_matrix.m[0][0]);

        if(drop_shadow)
        {
            Matrix model_shadow = {0};
            Vector3f pos_shadow = {-4.0*scale,4.0*scale,0.0};
            get_model_transform(&pos_shadow,&rot,&sca,&model_shadow);

            glUniformMatrix4fv(loc_font_model,1,GL_TRUE,&model_shadow.m[0][0]);
            glUniform4f(loc_font_fg_color,0.0,0.0,0.0,opacity);
            glDrawArrays(GL_TRIANGLES,0,6);
        }

        glUniformMatrix4fv(loc_font_model,1,GL_TRUE,&model.m[0][0]);
        glUniform4f(loc_font_fg_color,r/255.0,g/255.0,b/255.0,opacity);
        glDrawArrays(GL_TRIANGLES,0,6);

        x_pos += (fontsize*fc->advance);
        c++;
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);

    Vector2f ret = {x_pos - x, fontsize};
    return ret;
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






// static int assign_image2(GFXImage2** img)
// {
//     for(int i = 0; i < MAX_GFX_IMAGES; ++i)
//     {
//         if(gfx_images[i].texture == -1)
//         {
//             GFXImage* img = &gfx_images[i];

//             img->w = image->w;
//             img->h = image->h;
//             img->n = image->n;

//             LOGI("Assigning image to index: %d", i);

//             if(sub_image_data != NULL)
//             {
//                 img->sub_img_data = calloc(1,sizeof(GFXSubImageData));
//                 img->sub_img_data->element_count = sub_image_data->element_count;
//                 img->sub_img_data->elements_per_row = sub_image_data->elements_per_row;
//                 img->sub_img_data->elements_per_col = sub_image_data->elements_per_col;
//                 img->sub_img_data->element_width = sub_image_data->element_width;
//                 img->sub_img_data->element_height = sub_image_data->element_height;
//                 img->sub_img_data->visible_rects = calloc(img->sub_img_data->element_count, sizeof(Rect));
//                 img->sub_img_data->sprite_rects = calloc(img->sub_img_data->element_count, sizeof(Rect));
//                 for(int e = 0; e < img->sub_img_data->element_count; ++e)
//                 {
//                     memcpy(&img->sub_img_data->visible_rects[e], &sub_image_data->visible_rects[e], sizeof(Rect));
//                     memcpy(&img->sub_img_data->sprite_rects[e], &sub_image_data->sprite_rects[e], sizeof(Rect));
//                 }
//                 LOGI("Image set: width: %d, height: %d, count: %d", img->sub_img_data->element_width, img->sub_img_data->element_height, img->sub_img_data->element_count);
//             }
//             else
//             {
//                 gfx_get_image_visible_rect(img->w, img->h, img->n, image->data, &img->visible_rect);
//                 img->sprite_rect.w = img->visible_rect.w / img->w;
//                 img->sprite_rect.h = img->visible_rect.h / img->h;
//                 img->sprite_rect.x = (img->visible_rect.x) / img->w;
//                 img->sprite_rect.y = (img->visible_rect.y) / img->h;
//                 LOGI("Visible Rectangle: x: %.0f, y: %.0f, w: %.0f, h: %.0f", img->visible_rect.x, img->visible_rect.y, img->visible_rect.w, img->visible_rect.h);
//             }

//             glGenTextures(1, &img->texture);
//             glBindTexture(GL_TEXTURE_2D, img->texture);
//             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data);

//             if(linear_filter)
//             {
//                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//             }
//             else
//             {
//                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//             }

//             glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//             glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

//             glBindTexture(GL_TEXTURE_2D, 0);

//             if(img->texture == -1)
//                 img->texture = i; // @HACK

//             return i;
//         }
//     }

//     return -1;
// }

GFXImage2 gfx_images2[MAX_GFX_IMAGES] = {0};


int gfx_load_image2(const char* image_path, bool flip, bool linear_filter, int element_width, int element_height, GFXNodeDataInput* node_input)
{
    GFXImageData image = {0};
    bool load = gfx_load_image_data(image_path, &image, flip);
    if(!load) return -1;

    GFXImage2 img = {0};
    img.w = image.w;
    img.h = image.h;
    img.n = image.n;

    if(element_width <= 0 || element_height <= 0)
    {
        img.elements_per_row = 1;
        img.elements_per_col = 1;
        img.element_width = img.w;
        img.element_height = img.h;
    }
    else
    {
        img.elements_per_row = (img.w / element_width);
        img.elements_per_col = (img.h / element_height);
        img.element_width = element_width;
        img.element_height = element_height;
    }
    img.element_count = img.elements_per_row * img.elements_per_col;

    // printf("image wh: %d, %d\n", img.w, img.h);
    // printf("element count: %d  (%d x %d)\n", img.element_count, img.elements_per_row, img.elements_per_col);

    img.visible_rects = malloc(img.element_count * sizeof(Rect));
    img.sprite_rects = malloc(img.element_count * sizeof(Rect));

    int num_cols = img.elements_per_row;
    int num_rows = img.elements_per_col;

    bool nodes = false;
    GFXImageData node_image = {0};
    if(node_input != NULL)
    {
        node_input->num_sets = MIN(node_input->num_sets, 10);
        if(node_input->num_sets > 0)
        {
            nodes = gfx_load_image_data(node_input->image_path, &node_image, flip);

            if(nodes)
            {
                img.node_sets = node_input->num_sets;
                img.nodes = calloc(img.node_sets, sizeof(Vector2f*));
                for(int c = 0; c < img.node_sets; ++c)
                {
                    img.nodes[c] = calloc(img.element_count, sizeof(Vector2f));
                }
            }
        }
    }

    size_t temp_size = img.element_width*img.element_height*img.n*sizeof(unsigned char);
    unsigned char* temp_data = malloc(temp_size);

    for(int i = 0; i < img.element_count; ++i)
    {
        int start_x = (i % num_cols) * img.element_width;
        int start_y = (i / num_cols) * img.element_height;
        for(int y = 0; y < img.element_height; ++y)
        {
            for(int x = 0; x < img.element_width; ++x)
            {
                int index = ((start_y+y)*image.w + (start_x+x)) * img.n;
                int sub_index = (y*img.element_width + x) * img.n;
                for(int _n = 0; _n < img.n; ++_n)
                {
                    temp_data[sub_index+_n] = image.data[index+_n];
                }

                if(nodes)
                {
                    uint32_t color = COLOR(node_image.data[index+0], node_image.data[index+1], node_image.data[index+2]);
                    for(int c = 0; c < img.node_sets; ++c)
                    {
                        if(node_input->colors[c] == color)
                        {
                            img.nodes[c][i].x = x;
                            img.nodes[c][i].y = y;
                            // LOGI("Found node: %d,%d -> %.0f,%.0f", x, y, img.nodes[c][i].x, img.nodes[c][i].y);
                            continue;
                        }
                    }
                }

            }//element_width
        }//element_height

        gfx_get_image_visible_rect(img.element_width, img.element_height, img.n, temp_data, &img.visible_rects[i]);
        Rect* vr = &img.visible_rects[i];
        img.sprite_rects[i].x = (float)(start_x+vr->x) / image.w;
        img.sprite_rects[i].y = (float)(start_y+vr->y) / image.h;
        img.sprite_rects[i].w = vr->w / image.w;
        img.sprite_rects[i].h = vr->h / image.h;

        for(int c = 0; c < img.node_sets; ++c)
        {
            float nx = img.nodes[c][i].x;
            float ny = img.nodes[c][i].y;
            // node stays at center if not found for image
            if(!FEQ(nx,0.0) || !FEQ(ny,0.0))
            {
                img.nodes[c][i].x = nx - vr->x;
                img.nodes[c][i].y = ny - vr->y;
            }
            // LOGI("[%2d, %2d] Node: %.1f,%.1f -> %.1f,%.1f", c, i, nx, ny, img.nodes[c][i].x, img.nodes[c][i].y);
        }

    }//element_count


    //assign image
    for(int i = 0; i < MAX_GFX_IMAGES; ++i)
    {
        if(gfx_images2[i].texture == -1)
        {

            GFXImage2* p = &gfx_images2[i];
            memcpy(p, &img, sizeof(GFXImage2));

            glGenTextures(1, &p->texture);
            if(p->texture == -1)
            {
                p->texture = i; // @HACK: for server
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, p->texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.w, image.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);

                if(linear_filter)
                {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                }
                else
                {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                }

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glBindTexture(GL_TEXTURE_2D, 0);
            }


DEBUG_PRINT();
            if(temp_data != NULL) free(temp_data);
            if(image.data != NULL) free(image.data);
            if(node_image.data != NULL) free(node_image.data);
DEBUG_PRINT();

            return i;
            // return p->texture;
        }
    }
DEBUG_PRINT();

    if(temp_data != NULL) free(temp_data);
    if(image.data != NULL) free(image.data);
    if(node_image.data != NULL) free(node_image.data);

    return -1;
}


bool gfx_draw_image2(int img_index, int sprite_index, float x, float y, uint32_t color, float scale, float rotation, float opacity)
{
    if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
    {
        LOGE("%s: Invalid image index!", __func__);
        return false;
    }

    GFXImage2* img = &gfx_images2[img_index];

    if(sprite_index >= img->element_count)
    {
        LOGE("Invalid sprite index: %d (%d, %d)", sprite_index, img_index, img->element_count);
        return false;
    }
    float vw = img->visible_rects[sprite_index].w;
    float vh = img->visible_rects[sprite_index].h;


    glUseProgram(program_sprite);
    Rect* sr = &img->sprite_rects[sprite_index];
    float sprite_x = sr->x-sr->w/2.0;
    float sprite_y = sr->y-sr->h/2.0;
    float sprite_w = sr->w;
    float sprite_h = sr->h;

    Vertex quad[] =
    {
        {{-0.5, +0.5},{sprite_x,sprite_y+sprite_h}},
        {{+0.5, -0.5},{sprite_x+sprite_w,sprite_y}},
        {{-0.5, -0.5},{sprite_x,sprite_y}},
        {{-0.5, +0.5},{sprite_x,sprite_y+sprite_h}},
        {{+0.5, +0.5},{sprite_x+sprite_w,sprite_y+sprite_h}},
        {{+0.5, -0.5},{sprite_x+sprite_w,sprite_y}},
    };

    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);

    Matrix model = {0};

    Vector3f pos = {x,y,0.0};
    Vector3f rot = {0.0,0.0,360.0-rotation};
    Vector3f sca = {scale*vw,scale*vh,1.0};

    get_model_transform(&pos,&rot,&sca,&model);
    Matrix* view = get_camera_transform();

    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color >> 0;

    //printf("num_in_row: %d, sprite_index: %d\n",num_in_row, sprite_index);

    glUniform3f(loc_sprite_tint_color,r/255.0,g/255.0,b/255.0);
    glUniform1f(loc_sprite_opacity,opacity);
    glUniform1f(loc_sprite_brightness,1.0);

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

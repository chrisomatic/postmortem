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


typedef struct
{
    int w,h,n;
    unsigned char* data;
} _Image;

static bool load_image(const char* image_path, _Image* image, bool flip);
static int assign_image(GFXSubImageData* sub_image_data, _Image* image, bool linear_filter);

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
    font_image = gfx_load_image("core/fonts/atlas.png", false, true);
    printf("font image index: %d\n",font_image);

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

        printf("font_char %d: %f [%f %f %f %f], [%f %f %f %f], [%f %f %f %f]\n",char_index,advance,pl_l,pl_b,pl_r,pl_t,px_l,px_b,px_r,px_t,px_l/512.0,px_b/512.0,px_r/512.0,px_t/512.0);
    }

    fclose(fp);
}


void gfx_init(int width, int height)
{
    printf("GL version: %s\n",glGetString(GL_VERSION));

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
    loc_basic_model      = glGetUniformLocation(program_basic, "model");
    loc_basic_view       = glGetUniformLocation(program_basic, "view");
    loc_basic_proj       = glGetUniformLocation(program_basic, "projection");

    loc_sprite_image      = glGetUniformLocation(program_sprite, "image");
    loc_sprite_tint_color = glGetUniformLocation(program_sprite, "tint_color");
    loc_sprite_opacity    = glGetUniformLocation(program_sprite, "opacity");
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

    stbi_set_flip_vertically_on_load(0);

    for(int i = 0; i < MAX_GFX_IMAGES; ++i)
    {
        gfx_images[i].texture = -1;
    }

    load_font();
}

void gfx_clear_buffer(uint8_t r, uint8_t g, uint8_t b)
{
    glClearColor(r/255.0, g/255.0,b/255.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

static bool load_image(const char* image_path, _Image* image, bool flip)
{
    printf("%s\n",image_path);

    stbi_set_flip_vertically_on_load(flip);
    image->data = stbi_load(image_path,&image->w,&image->h,&image->n,4);

    if(image->data != NULL)
    {
        printf("Loaded image: %s (x: %d, y: %d, n: %d)\n", image_path, image->w, image->h, image->n);
         return true;
    }
    else
    {
        printf("Image data is NULL: %s\n", image_path);
        return false;
    }
}

static int assign_image(GFXSubImageData* sub_image_data, _Image* image, bool linear_filter)
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
                img->sub_img_data->sprite_rects = calloc(img->sub_img_data->element_count, sizeof(Rect));
                for(int e = 0; e < img->sub_img_data->element_count; ++e)
                {
                    memcpy(&img->sub_img_data->visible_rects[e], &sub_image_data->visible_rects[e], sizeof(Rect));
                    memcpy(&img->sub_img_data->sprite_rects[e], &sub_image_data->sprite_rects[e], sizeof(Rect));
                }

                printf("Image set: width: %d, height: %d, count: %d\n", img->sub_img_data->element_width, img->sub_img_data->element_height, img->sub_img_data->element_count);
            }
            else
            {
                gfx_get_image_visible_rect(img->w, img->h, img->n, image->data, &img->visible_rect);

                img->sprite_rect.x = img->visible_rect.x / img->w;
                img->sprite_rect.y = img->visible_rect.y / img->h;
                img->sprite_rect.w = img->visible_rect.w / img->w;
                img->sprite_rect.h = img->visible_rect.h / img->h;

                printf("Visible Rectangle: x: %.0f, y: %.0f, w: %.0f, h: %.0f\n", img->visible_rect.x, img->visible_rect.y, img->visible_rect.w, img->visible_rect.h);
            }

            glGenTextures(1, &img->texture);
            glBindTexture(GL_TEXTURE_2D, img->texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data);

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
            return i;
        }
    }

    return -1;
}

int gfx_load_image(const char* image_path, bool flip, bool linear_filter)
{
    _Image image = {0};
    bool load = load_image(image_path, &image, flip);
    if(!load) return -1;

    int idx = assign_image(NULL, &image, linear_filter);

    stbi_image_free(image.data);

    return idx;
}

int gfx_load_image_set(const char* image_path, int element_width, int element_height)
{
    _Image image = {0};
    bool load = load_image(image_path, &image, false);
    if(!load) return -1;

    int num_cols = (image.w / element_width);     //cols
    int num_rows = (image.h / element_height);    //rows
    int element_count = num_cols*num_rows;

    GFXSubImageData sid = {0};
    sid.element_width = element_width;
    sid.element_height = element_height;
    sid.element_count = element_count;
    sid.visible_rects = malloc(sid.element_count * sizeof(Rect));
    sid.sprite_rects = malloc(sid.element_count * sizeof(Rect));

    int n = image.n;
    unsigned char* temp_data = malloc(element_width*element_height*n*sizeof(unsigned char));

    for(int i = 0; i < sid.element_count; ++i)
    {
        int start_x = (i % num_cols) * element_width;
        int start_y = (i / num_cols) * element_height;
        for(int y = 0; y < element_height; ++y)
        {
            for(int x = 0; x < element_width; ++x)
            {
                int index = ((start_y+y)*image.w + (start_x+x)) * n;
                int sub_index = (y*element_width + x) * n;
                temp_data[sub_index] = image.data[index];
                temp_data[sub_index+1] = image.data[index+1];
                temp_data[sub_index+2] = image.data[index+2];
                temp_data[sub_index+3] = image.data[index+3];
            }
        }


        gfx_get_image_visible_rect(element_width, element_height, 4, temp_data, &sid.visible_rects[i]);

        Rect* vr = &sid.visible_rects[i];
        sid.sprite_rects[i].x = (float)(start_x+vr->x) / image.w;
        sid.sprite_rects[i].y = (float)(start_y+vr->y) / image.h;
        sid.sprite_rects[i].w = vr->w / image.w;
        sid.sprite_rects[i].h = vr->h / image.h;

    }

    int idx = assign_image(&sid, &image, false);

    free(temp_data);
    stbi_image_free(image.data);
    free(sid.visible_rects);
    free(sid.sprite_rects);

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
        if(sid->sprite_rects != NULL) free(sid->sprite_rects);
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

    float vw = img->visible_rect.w;
    float vh = img->visible_rect.h;

    Rect* sr = &img->sprite_rect;
    float sprite_x = sr->x;
    float sprite_y = sr->y;
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

    Vector3f pos = {x+vw/2.0,y+vh/2.0,0.0};
    Vector3f rot = {0.0,0.0,360.0-rotation};
    Vector3f sca = {scale*vw,scale*vh,1.0};

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
    float vw = sid->visible_rects[sprite_index].w;
    float vh = sid->visible_rects[sprite_index].h;


    glUseProgram(program_sprite);
    Rect* sr = &sid->sprite_rects[sprite_index];
    float sprite_x = sr->x;
    float sprite_y = sr->y;
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

    Vector3f pos = {x+vw/2.0,y+vh/2.0,0.0};
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

void gfx_string_get_size(char* str, float scale, float* w, float* h)
{
    float tallest_h = 0.0;
    float x_pos = 0.0;

    char* c = str;

    for(;;)
    {
        if(*c == '\0')
            break;

        FontChar* fc = &font_chars[*c];

        float h = fc->h*scale;
        if(ABS(h) > tallest_h)
            tallest_h = ABS(h);

        x_pos += scale*(fc->w + fc->advance);
        c++;
    }

    if(w) *w = x_pos;
    if(h) *h = tallest_h;
}

void gfx_stringf_get_size(float scale, float* w, float* h, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char str[256] = {0};
    vsprintf(str,fmt, args);
    gfx_string_get_size(str, scale, w, h);
    va_end(args);
}

void gfx_draw_string(char* str, float x, float y, uint32_t color, float scale, float rotation, float opacity, bool in_world, bool drop_shadow)
{
    glUseProgram(program_font);

    Matrix* view = get_camera_transform();

    GFXImage* img = &gfx_images[font_image];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, img->texture);
    //glBindTexture(GL_TEXTURE_2D, gfx_images[1].texture);
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

    float x_pos = x;

    // used to draw the string at top left of first character
    float tallest_h = 0.0;

    for(;;)
    {
        if(*c == '\0')
            break;

        FontChar* fc = &font_chars[*c];

        float h = fc->h*scale;
        if(ABS(h) > tallest_h)
            tallest_h = ABS(h);

        c++;
    }


    if(drop_shadow)
    {
        glUniform4f(loc_font_fg_color,0.2,0.2,0.2,opacity);

        c = str;

        for(;;)
        {
            if(*c == '\0')
                break;

            FontChar* fc = &font_chars[*c];

            float x_loc = x_pos+(fc->w*scale/2.0)+(fc->plane_box.l*fc->w*scale) -0.7;
            float y_loc = y+tallest_h+(fc->h*scale/2.0)+(fc->plane_box.b*fc->h*scale)+0.7;

            Vector3f pos = {x_loc,y_loc,0.0};
            Vector3f rot = {0.0,0.0,rotation};
            Vector3f sca = {scale*fc->w,-scale*fc->h,1.0};

            Matrix model = {0};
            get_model_transform(&pos,&rot,&sca,&model);

            Vertex fontchar[] =
            {
                {{-0.5, +0.5},{fc->tex_coords.l,fc->tex_coords.b}},
                {{+0.5, -0.5},{fc->tex_coords.r,fc->tex_coords.t}},
                {{-0.5, -0.5},{fc->tex_coords.l,fc->tex_coords.t}},
                {{-0.5, +0.5},{fc->tex_coords.l,fc->tex_coords.b}},
                {{+0.5, +0.5},{fc->tex_coords.r,fc->tex_coords.b}},
                {{+0.5, -0.5},{fc->tex_coords.r,fc->tex_coords.t}},
            };
            
            //printf("%c: [%f %f, %f, %f %f %f %f]\n",*c, fc->w, fc->h, fc->advance, fc->plane_box.l, fc->plane_box.b, fc->plane_box.r, fc->plane_box.t);

            glBindBuffer(GL_ARRAY_BUFFER, font_vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fontchar), fontchar);
            glUniformMatrix4fv(loc_font_model,1,GL_TRUE,&model.m[0][0]);

            if(in_world)
                glUniformMatrix4fv(loc_font_view,1,GL_TRUE,&view->m[0][0]);
            else
                glUniformMatrix4fv(loc_font_view,1,GL_TRUE,&IDENTITY_MATRIX.m[0][0]);

            glUniformMatrix4fv(loc_font_proj,1,GL_TRUE,&proj_matrix.m[0][0]);

            glDrawArrays(GL_TRIANGLES,0,6);//,GL_UNSIGNED_INT,0);

            if(*c == ' ')
                x_pos += 16*scale;
            else
                x_pos += scale*(fc->w + fc->advance);

            c++;
        }

    }

    glUniform4f(loc_font_fg_color,r/255.0,g/255.0,b/255.0,opacity);

    c = str;
    x_pos = x;
    for(;;)
    {
        if(*c == '\0')
            break;

        FontChar* fc = &font_chars[*c];

        float x_loc = x_pos+(fc->w*scale/2.0)+(fc->plane_box.l*fc->w*scale);
        float y_loc = y+tallest_h+(fc->h*scale/2.0)+(fc->plane_box.b*fc->h*scale);

        Vector3f pos = {x_loc,y_loc,0.0};
        Vector3f rot = {0.0,0.0,rotation};
        Vector3f sca = {scale*fc->w,-scale*fc->h,1.0};

        Matrix model = {0};
        get_model_transform(&pos,&rot,&sca,&model);

        Vertex fontchar[] =
        {
            {{-0.5, +0.5},{fc->tex_coords.l,fc->tex_coords.b}},
            {{+0.5, -0.5},{fc->tex_coords.r,fc->tex_coords.t}},
            {{-0.5, -0.5},{fc->tex_coords.l,fc->tex_coords.t}},
            {{-0.5, +0.5},{fc->tex_coords.l,fc->tex_coords.b}},
            {{+0.5, +0.5},{fc->tex_coords.r,fc->tex_coords.b}},
            {{+0.5, -0.5},{fc->tex_coords.r,fc->tex_coords.t}},
        };
        
        //printf("%c: [%f %f, %f, %f %f %f %f]\n",*c, fc->w, fc->h, fc->advance, fc->plane_box.l, fc->plane_box.b, fc->plane_box.r, fc->plane_box.t);

        glBindBuffer(GL_ARRAY_BUFFER, font_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fontchar), fontchar);
        glUniformMatrix4fv(loc_font_model,1,GL_TRUE,&model.m[0][0]);

        if(in_world)
            glUniformMatrix4fv(loc_font_view,1,GL_TRUE,&view->m[0][0]);
        else
            glUniformMatrix4fv(loc_font_view,1,GL_TRUE,&IDENTITY_MATRIX.m[0][0]);

        glUniformMatrix4fv(loc_font_proj,1,GL_TRUE,&proj_matrix.m[0][0]);

        glDrawArrays(GL_TRIANGLES,0,6);//,GL_UNSIGNED_INT,0);

        if(*c == ' ')
            x_pos += 16*scale;
        else
            x_pos += scale*(fc->w + fc->advance);

        c++;
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);
}

void gfx_draw_stringf(float x, float y, uint32_t color, float scale, float rotation, float opacity, bool in_world, bool drop_shadow, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char str[256] = {0};
    vsprintf(str,fmt, args);
    gfx_draw_string(str, x, y, color, scale, rotation, opacity, in_world, drop_shadow);
    va_end(args);
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

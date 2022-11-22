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
#include "lighting.h"
#include "log.h"
#include "gfx.h"


// types
// --------------------------------------------------------
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

// static vars
// --------------------------------------------------------
static GLuint quad_vao, quad_vbo;
static GLuint rect_vao, rect_vbo;
static GLuint font_vao, font_vbo;
static GLuint line_vao,line_vbo;

static Matrix proj_matrix;

static GLint loc_sprite_image;
static GLint loc_sprite_ambient_color;
static GLint loc_sprite_tint_color;
static GLint loc_sprite_is_particle;
static GLint loc_sprite_opacity;
static GLint loc_sprite_model;
static GLint loc_sprite_view;
static GLint loc_sprite_proj;
static GLint loc_sprite_light_pos[MAX_POINT_LIGHTS];
static GLint loc_sprite_light_color[MAX_POINT_LIGHTS];
static GLint loc_sprite_light_atten[MAX_POINT_LIGHTS];

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
static GLint loc_font_tex;
static GLint loc_font_px_range;
static GLint loc_font_opacity;
static GLint loc_font_model;
static GLint loc_font_view;
static GLint loc_font_proj;

#define MAX_LINES 100
static LinePoint line_points[2*MAX_LINES];
static int num_line_points = 0;

static FontChar font_chars[255];


static double vr_time = 0;
static double img_time = 0;
static double other_time = 0;


// global vars
// --------------------------------------------------------
GFXImage gfx_images[MAX_GFX_IMAGES] = {0};
int font_image;


// macros
// --------------------------------------------------------
// #define DEBUG_PRINT()   printf("%d %s %s()\n", __LINE__, __FILE__, __func__)


// static function prototypes
// --------------------------------------------------------
static void load_font();
static int image_find_first_visible_rowcol(int side, int img_w, int img_h, int img_n, unsigned char* data);
static void image_get_visible_rect(int img_w, int img_h, int img_n, unsigned char* img_data, Rect* ret);


// global functions
// --------------------------------------------------------

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

    // rect (used for drawing empty rectangles
    glGenVertexArrays(1, &rect_vao);
    glBindVertexArray(rect_vao);

    Vector2f rect[] =
    {
        {-0.5, -0.5},
        {-0.5, +0.5},
        {+0.5, +0.5},
        {+0.5, -0.5},
        {-0.5, -0.5},
    };

    glGenBuffers(1, &rect_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rect_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rect), rect, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vector2f), (void*)0);

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
    loc_sprite_image      = glGetUniformLocation(program_sprite, "image");
    loc_sprite_ambient_color = glGetUniformLocation(program_sprite, "ambient_color");
    loc_sprite_tint_color = glGetUniformLocation(program_sprite, "tint_color");
    loc_sprite_is_particle = glGetUniformLocation(program_sprite, "is_particle");

    char lookup_str[16+1] = {0};
    for(int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        snprintf(lookup_str,16,"light_pos[%d]",i);
        loc_sprite_light_pos[i]     = glGetUniformLocation(program_sprite, lookup_str);
        snprintf(lookup_str,16,"light_color[%d]",i);
        loc_sprite_light_color[i]   = glGetUniformLocation(program_sprite, lookup_str);
        snprintf(lookup_str,16,"light_atten[%d]",i);
        loc_sprite_light_atten[i]   = glGetUniformLocation(program_sprite, lookup_str);
    }

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
    loc_font_px_range = glGetUniformLocation(program_font, "px_range");
    loc_font_tex      = glGetUniformLocation(program_font, "tex");
    loc_font_model    = glGetUniformLocation(program_font, "model");
    loc_font_view     = glGetUniformLocation(program_font, "view");
    loc_font_proj     = glGetUniformLocation(program_font, "projection");

    ortho(&proj_matrix,0.0,(float)width,(float)height,0.0, 0.0, 1000.0);

    print_matrix(&proj_matrix);

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


void gfx_image_init()
{
    stbi_set_flip_vertically_on_load(0);

    for(int i = 0; i < MAX_GFX_IMAGES; ++i)
    {
        gfx_images[i].texture = -1;
    }
}

bool gfx_load_image_data(const char* image_path, GFXImageData* image, bool flip)
{
    // LOGI("Loading image: %s",image_path);

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

int gfx_load_image(const char* image_path, bool flip, bool linear_filter, int element_width, int element_height, GFXNodeDataInput* node_input)
{
    GFXImageData image = {0};
    
    Timer _timer = {0};
    timer_begin(&_timer);   //img_time
    bool load = gfx_load_image_data(image_path, &image, flip);
    img_time += timer_get_elapsed(&_timer);

    if(!load) return -1;

    timer_begin(&_timer);   //other_time

    GFXImage img = {0};
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

    LOGI("  Element Count: %d (%d x %d)", img.element_count, img.elements_per_row, img.elements_per_col);

    img.visible_rects = malloc(img.element_count * sizeof(Rect));
    img.sprite_visible_rects = malloc(img.element_count * sizeof(Rect));
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
                img.node_colors = calloc(img.node_sets, sizeof(uint32_t));
                img.nodes = calloc(img.node_sets, sizeof(Vector2f*));
                for(int c = 0; c < img.node_sets; ++c)
                {
                    img.node_colors[c] = node_input->colors[c];
                    img.nodes[c] = calloc(img.element_count, sizeof(Vector2f));
                    for(int i = 0; i < img.element_count; ++i)
                    {
                        img.nodes[c][i].x = INFINITY;
                        img.nodes[c][i].y = INFINITY;
                    }
                }
            }
        }
    }



    size_t temp_size = img.element_width*img.element_height*img.n*sizeof(unsigned char);
    unsigned char* temp_data = malloc(temp_size);

    other_time += timer_get_elapsed(&_timer);

    for(int i = 0; i < img.element_count; ++i)
    {
        timer_begin(&_timer);   //other_time

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
                        if(img.node_colors[c] == color)
                        {
                            img.nodes[c][i].x = x;
                            img.nodes[c][i].y = y;
                            // LOGI("(0x%08x) Found node at: %d,%d", color, img.nodes[c][i].x, img.nodes[c][i].y);
                            continue;
                        }
                    }
                }

            }//element_width
        }//element_height

        other_time += timer_get_elapsed(&_timer);

        image_get_visible_rect(img.element_width, img.element_height, img.n, temp_data, &img.visible_rects[i]);
        Rect* vr = &img.visible_rects[i];
        img.sprite_visible_rects[i].x = (float)(start_x+vr->x) / image.w;
        img.sprite_visible_rects[i].y = (float)(start_y+vr->y) / image.h;
        img.sprite_visible_rects[i].w = vr->w / image.w;
        img.sprite_visible_rects[i].h = vr->h / image.h;

        img.sprite_rects[i].x = (float)(start_x+img.element_width/2.0) / image.w;
        img.sprite_rects[i].y = (float)(start_y+img.element_height/2.0) / image.h;
        img.sprite_rects[i].w = (float)img.element_width / image.w;
        img.sprite_rects[i].h = (float)img.element_height / image.h;


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


    timer_begin(&_timer);   //other_time

    //assign image
    for(int i = 0; i < MAX_GFX_IMAGES; ++i)
    {
        if(gfx_images[i].texture == -1)
        {
            LOGI("  Index: %d", i);

            GFXImage* p = &gfx_images[i];
            memcpy(p, &img, sizeof(GFXImage));

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


            // DEBUG
            // if(p->texture == 17)
            // {
            //     for(int idx = 0; idx < img.element_count; ++idx)
            //     {
            //         printf("%d) ", idx);
            //         print_rect(&p->visible_rects[idx]);
            //     }
            // }


            if(temp_data != NULL) free(temp_data);
            if(image.data != NULL) free(image.data);
            if(node_image.data != NULL) free(node_image.data);

            other_time += timer_get_elapsed(&_timer);

            return i;
            // return p->texture;
        }
    }

    if(temp_data != NULL) free(temp_data);
    if(image.data != NULL) free(image.data);
    if(node_image.data != NULL) free(node_image.data);

    return -1;
}

static bool gfx_draw_image_internal(int img_index, int sprite_index, float x, float y, uint32_t color, float scale, float rotation, float opacity, bool full_image, bool in_world, bool is_particle)
{
    if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
    {
        LOGE("%s: Invalid image index!", __func__);
        return false;
    }

    GFXImage* img = &gfx_images[img_index];

    if(sprite_index >= img->element_count)
    {
        LOGE("Invalid sprite index: %d (%d, %d)", sprite_index, img_index, img->element_count);
        return false;
    }

    glUseProgram(program_sprite);

    Rect* sr = NULL;
    if(full_image)
        sr = &img->sprite_rects[sprite_index];
    else
        sr = &img->sprite_visible_rects[sprite_index];

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
    Vector3f sca = {1.0,1.0,1.0};

    if(full_image)
    {
        // Vector3f sca = {scale*img->element_width,scale*img->element_height,1.0};
        sca.x = scale*img->element_width;
        sca.y = scale*img->element_height;
    }
    else
    {
        // Vector3f sca = {scale*vw,scale*vh,1.0};
        float vw = img->visible_rects[sprite_index].w;
        float vh = img->visible_rects[sprite_index].h;
        sca.x = scale*vw;
        sca.y = scale*vh;
    }

    get_model_transform(&pos,&rot,&sca,&model);
    Matrix* view = get_camera_transform();

    /*
    if(img_index == 3)
    {
        Matrix r;
        dot_product_mat(*view, model, &r);
        printf("===========\n");
        printf("view-model\n");
        print_matrix(&r);
        printf("===========\n");
        printf("view-model-proj\n");
        Matrix vmp;
        dot_product_mat(proj_matrix,r,&vmp);
        print_matrix(&vmp);
    }
    */

    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color >> 0;

    //printf("num_in_row: %d, sprite_index: %d\n",num_in_row, sprite_index);

    float brightness = 1.0;

    glUniform3f(loc_sprite_tint_color,brightness*r/255.0,brightness*g/255.0,brightness*b/255.0);
    glUniform3f(loc_sprite_ambient_color,r/255.0,g/255.0,b/255.0);//0.4,0.4,0.4);
    glUniform1i(loc_sprite_is_particle,(is_particle ? 1 : 0));

    for(int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        if(i < point_light_count)
        {
            PointLight* pl = &point_lights[i];
            glUniform2f(loc_sprite_light_pos[i],pl->pos.x,pl->pos.y);
            glUniform3f(loc_sprite_light_color[i],pl->color.x,pl->color.y,pl->color.z);
            glUniform3f(loc_sprite_light_atten[i],pl->attenuation.x,pl->attenuation.y,pl->attenuation.z);
        }
        else
        {
            glUniform2f(loc_sprite_light_pos[i],0.0,0.0);
            glUniform3f(loc_sprite_light_color[i],0.0,0.0,0.0);
            glUniform3f(loc_sprite_light_atten[i],1.0,0.0,0.0);
        }
    }

    glUniform1f(loc_sprite_opacity,opacity);

    glUniformMatrix4fv(loc_sprite_model,1,GL_TRUE,&model.m[0][0]);

    if(in_world)
        glUniformMatrix4fv(loc_sprite_view,1,GL_TRUE,&view->m[0][0]);
    else
        glUniformMatrix4fv(loc_sprite_view,1,GL_TRUE,&IDENTITY_MATRIX.m[0][0]);

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

bool gfx_draw_image(int img_index, int sprite_index, float x, float y, uint32_t color, float scale, float rotation, float opacity, bool full_image, bool in_world)
{
    return gfx_draw_image_internal(img_index, sprite_index, x, y, color, scale, rotation, opacity, full_image, in_world, false);

}

bool gfx_draw_particle(int img_index, int sprite_index, float x, float y, uint32_t color, float scale, float rotation, float opacity, bool full_image, bool in_world)
{
    return gfx_draw_image_internal(img_index, sprite_index, x, y, color, scale, rotation, opacity, full_image, in_world, true);
}

GFXImage* gfx_get_image_data(int img_index)
{
    if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
    {
        LOGE("%s: Invalid image index!", __func__);
        return NULL;
    }
    return &gfx_images[img_index];
}

bool gfx_get_image_node_point(int img_index, int sprite_index, uint32_t node_color, Vector2f* node)
{
    if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
    {
        LOGE("%s: Invalid image index!", __func__);
        return false;
    }

    GFXImage* img = &gfx_images[img_index];
    if(sprite_index >= img->element_count)
    {
        LOGE("Invalid sprite index: %d (%d, %d)", sprite_index, img_index, img->element_count);
        return false;
    }

    for(int c = 0; c < img->node_sets; ++c)
    {
        if(img->node_colors[c] == node_color)
        {
            bool xinf = img->nodes[c][sprite_index].x == INFINITY;
            bool yinf = img->nodes[c][sprite_index].y == INFINITY;
            if(xinf || yinf) return false;
            if(node != NULL)
            {
                node->x = img->nodes[c][sprite_index].x;
                node->y = img->nodes[c][sprite_index].y;
            }
            return true;
        }
    }
    return false;
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

void gfx_draw_rect(Rect* r, uint32_t color, float rotation, float scale, float opacity, bool filled, bool in_world)
{
    gfx_draw_rect_xywh(r->x, r->y, r->w, r->h, color, rotation, scale, opacity, filled, in_world);
}

void gfx_draw_rect_xywh(float x, float y, float w, float h, uint32_t color, float rotation, float scale, float opacity, bool filled, bool in_world)
{
    glUseProgram(program_shape);

    Matrix model = {0};

    Vector3f pos = {x,y,0.0};
    Vector3f rot = {0.0,0.0,rotation};
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
        //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(quad_vao);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_TRIANGLES,0,6);
        glDisableVertexAttribArray(0);
    }
    else
    {
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBindVertexArray(rect_vao);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_LINE_STRIP,0,5);
        glDisableVertexAttribArray(0);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);
}

static Vector2f gfx_draw_string_internal(float x, float y, uint32_t color, uint32_t background_color, float scale, float rotation, float opacity, bool in_world, bool drop_shadow, char* str)
{
    glUseProgram(program_font);

    Matrix* view = get_camera_transform();

    GFXImage* img = &gfx_images[font_image];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, img->texture);
    glUniform1i(loc_font_image, 0);

    glBindVertexArray(font_vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    uint8_t br = background_color >> 16;
    uint8_t bg = background_color >> 8;
    uint8_t bb = background_color >> 0;

    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color >> 0;

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

Vector2f gfx_draw_string_with_background(float x, float y, uint32_t color, uint32_t background_color, float scale, float rotation, float opacity, bool in_world, bool drop_shadow, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char str[256] = {0};
    vsprintf(str,fmt, args);
    va_end(args);

    Vector2f size = gfx_string_get_size(scale, str);
    if(size.x > 0.0 && size.y > 0.0)
    {
        gfx_draw_rect_xywh(x+size.x/2.0, y+size.y/2.0, size.x+4, size.y+4, background_color, rotation, 1.0, opacity, true, in_world);
    }

    return gfx_draw_string_internal(x,y,color,background_color, scale, rotation, opacity, in_world, drop_shadow, str);
}

Vector2f gfx_draw_string(float x, float y, uint32_t color, float scale, float rotation, float opacity, bool in_world, bool drop_shadow, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char str[256] = {0};
    vsprintf(str,fmt, args);
    va_end(args);

    return gfx_draw_string_internal(x,y,color,0x00000000, scale, rotation, opacity, in_world, drop_shadow, str);
}

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
    anim->max_frame_time = RANGE(anim->max_frame_time, 0.001, 10);

    while(anim->curr_frame_time >= anim->max_frame_time)
    {
        anim->curr_frame_time -= anim->max_frame_time;
        // anim->curr_frame_time = 0;
        anim->curr_frame++;

        if(anim->curr_frame >= anim->max_frames)
        {
            anim->curr_frame = 0;

            // if(anim->finite)
            anim->curr_loop++;
        }
    }
}

// Misc

void gfx_color2floats(uint32_t color, float* r, float* g, float* b)
{
    *r = ((color >> 16) & 0xFF)/255.0f;
    *g = ((color >>  8) & 0xFF)/255.0f;
    *b = ((color >>  0) & 0xFF)/255.0f;
}

void gfx_print_times()
{
    printf("Image load time:   %.4f\n", img_time);
    printf("Visible rect time: %.4f\n", vr_time);
    printf("Other time:        %.4f\n", other_time);
}



// static functions
// --------------------------------------------------------

// get first row or col that's not empty
// side: 0=top,1=left,2=bottom,3=right
static int image_find_first_visible_rowcol(int side, int img_w, int img_h, int img_n, unsigned char* data)
{
    if(side == 0 || side == 2)
    {
        bool row_empty = true;
        bool prior_empty = true;
        for(int _y = 0; _y < img_h; ++_y)
        {

            int y = _y;
            // change scan direction
            if(side == 2)
                y = img_h - _y - 1;

            row_empty = true;

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

        // if this happens, then the entire image is blank
        if(row_empty)
        {
            return -1;
        }

        // I don't think this code ever gets hit
        if(side == 2) return img_h-1;  //bottom
        return 0;   //top
    }
    else if(side == 1 || side == 3)
    {
        bool col_empty = true;
        bool prior_empty = true;
        for(int _x = 0; _x < img_w; ++_x)
        {
            int x = _x;
            if(side == 3)
                x = img_w-_x-1;

            col_empty = true;

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

        // if this happens, then the entire image is blank
        if(col_empty)
        {
            return -1;
        }

        // I don't think this code ever gets hit
        if(side == 3) return img_w-1;
        return 0;
    }
    return -1;
}

static void image_get_visible_rect(int img_w, int img_h, int img_n, unsigned char* img_data, Rect* ret)
{
    Timer _timer = {0};
    timer_begin(&_timer);

    int top = image_find_first_visible_rowcol(0, img_w, img_h, img_n, img_data);

    // image is blank
    if(top == -1)
    {
        // printf("blank image\n");
        ret->w = 0;
        ret->h = 0;
        ret->x = img_w/2.0;
        ret->y = img_h/2.0;
        vr_time += timer_get_elapsed(&_timer);
        // printf("vr time: %.4f\n", vr_time);
        return;
    }

    int bottom = image_find_first_visible_rowcol(2, img_w, img_h, img_n, img_data);
    int left = image_find_first_visible_rowcol(1, img_w, img_h, img_n, img_data);
    int right = image_find_first_visible_rowcol(3, img_w, img_h, img_n, img_data);

    int height = bottom - top + 1;  //top left is origin
    int width = right - left + 1;

    // if(print_debug)
    // {
    //     printf("top: %d\n", top);
    //     printf("bot: %d\n", bottom);
    //     printf("lef: %d\n", left);
    //     printf("rig: %d\n", right);
    //     printf("w:   %d\n", width);
    //     printf("h:   %d\n", height);
    // }

    ret->w = (float)width;
    ret->h = (float)height;
    ret->x = (float)left + ret->w/2.0;
    ret->y = (float)top + ret->h/2.0;

    vr_time += timer_get_elapsed(&_timer);
    // printf("vr time: %.4f\n", vr_time);
}

static void load_font()
{
    font_image = gfx_load_image("core/fonts/atlas.png", false, true, 0, 0, NULL);
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

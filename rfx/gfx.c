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
#include "gfx.h"
#include "camera.h"
#include "rat_math.h"

static GLuint vao, vbo, ibo;

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

void gfx_init(int width, int height)
{
    printf("GL version: %s\n",glGetString(GL_VERSION));

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    Vertex vertices[6] =
    {
        {{-0.5, +0.5},{0.0,1.0}},
        {{+0.5, -0.5},{1.0,0.0}},
        {{-0.5, -0.5},{0.0,0.0}},
        {{-0.5, +0.5},{0.0,1.0}},
        {{+0.5, +0.5},{1.0,1.0}},
        {{+0.5, -0.5},{1.0,0.0}},
    }; 

    //uint32_t indices[6] = {0,1,2,2,0,3};

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /*
    glGenBuffers(1,&ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(uint32_t), indices, GL_STATIC_DRAW);
    */

    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vertex), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex), (const GLvoid*)8);

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

    /*
    printf("%d %d %d %d %d %d %d\n",
            loc_sprite_image,
            loc_sprite_tint_color,
            loc_sprite_opacity,
            loc_sprite_model,
            loc_sprite_proj,
            loc_sprite_num_in_row,
            loc_sprite_index);
            */

    ortho(&proj_matrix,0.0,(float)width,(float)height,0.0, -1.0, 1.0);

    //print_matrix(&proj_matrix);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

int gfx_load_image(const char* image_path)
{
    for(int i = 0; i < MAX_GFX_IMAGES; ++i)
    {
        if(gfx_images[i].texture == -1)
        {
            GFXImage* img = &gfx_images[i];
            unsigned char* data = stbi_load(image_path,&img->w,&img->h,&img->n,4);

            if(data == NULL)
                return -1;

            glGenTextures(1, &img->texture);
            glBindTexture(GL_TEXTURE_2D, img->texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindTexture(GL_TEXTURE_2D, 0);
            stbi_image_free(data);

            printf("Loaded image: %s (x: %d, y: %d, n: %d)\n",image_path,img->w,img->h,img->n);
            return i;
        }
    }

    return -1;
}

int gfx_load_image_set(const char* image_path, int element_width, int element_height)
{
    int img = gfx_load_image(image_path);

    gfx_images[img].is_set = true;
    gfx_images[img].element_width = element_width;
    gfx_images[img].element_height = element_height;

    return img;
}

void gfx_free_image(int img_index)
{
    if(img_index < 0 || img_index >= MAX_GFX_IMAGES)
    {
        printf("%s: Invalid image index!\n", __func__);
        return;
    }
    GFXImage* img = &gfx_images[img_index];
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

void gfx_draw_rect(float x, float y, float w, float h, uint32_t color, float scale, float opacity)
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

    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_TRIANGLES,0,6);//,GL_UNSIGNED_INT,0);

    glDisableVertexAttribArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);
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
    Vector3f rot = {0.0,0.0,rotation};
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

    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES,0,6);//,GL_UNSIGNED_INT,0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

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

    glUseProgram(program_sprite);

    Matrix model = {0};

    Vector3f pos = {x+img->element_width/2.0,y+img->element_height/2.0,0.0};
    Vector3f rot = {0.0,0.0,rotation};
    Vector3f sca = {scale*img->element_width,-scale*img->element_height,1.0};

    get_model_transform(&pos,&rot,&sca,&model);
    Matrix* view = get_camera_transform();

    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color >> 0;

    int num_in_row = (img->w / img->element_width); 
    int num_in_col = (img->h / img->element_height); 
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

    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES,0,6);//,GL_UNSIGNED_INT,0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);

    return true;

}

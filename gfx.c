#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <GL/glew.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "util/stb_image.h"

#include "shader.h"
#include "gfx.h"
#include "rat_math.h"

#define MAX_GFX_IMAGES 32

static GLuint texture = 0;
static GLuint vao, vbo, ibo;

typedef struct
{
    unsigned char* data;
    GLuint texture;
    int w,h,n;
} GFXImage;

GFXImage gfx_images[MAX_GFX_IMAGES] = {0};
int gfx_image_count = 0;

static Matrix proj_matrix;

static GLint location_image = 0;
static GLint location_tint_color = 0;
static GLint location_opacity = 0;
static GLint location_model = 0;
static GLint location_proj = 0;

void gfx_init(int width, int height)
{
    printf("GL version: %s\n",glGetString(GL_VERSION));

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    Vertex vertices[4] =
    {
        {{-0.5, -0.5},{0.0,0.0}},
        {{-0.5, 0.5},{0.0,1.0}},
        {{0.5,  0.5},{1.0,1.0}},
        {{0.5, -0.5},{1.0,0.0}},
    }; 

    uint32_t indices[6] = {0,1,2,2,0,3};

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 4*sizeof(Vertex), vertices, GL_STATIC_DRAW);

    glGenBuffers(1,&ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(uint32_t), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vertex), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex), (const GLvoid*)8);

    location_image = glGetUniformLocation(program, "image");
    location_tint_color = glGetUniformLocation(program, "tint_color");
    location_opacity = glGetUniformLocation(program, "opacity");
    location_model = glGetUniformLocation(program, "model");
    location_proj = glGetUniformLocation(program, "projection");

    printf("%d %d %d %d %d\n",location_image, location_tint_color, location_opacity, location_model, location_proj);

    glUniform1i(location_image, 0);

    ortho(&proj_matrix,0.0,width,height,0.0,-1.0,1.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    stbi_set_flip_vertically_on_load(1);

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
        if(gfx_images[i].data == NULL)
        {

            GFXImage* img = &gfx_images[i];
            img->data = stbi_load(image_path,&img->w,&img->h,&img->n,4);

            if(img->data == NULL)
                return -1;

            glGenTextures(1, &img->texture);
            glBindTexture(GL_TEXTURE_2D, img->texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindTexture(GL_TEXTURE_2D, 0);

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

bool gfx_draw_image(int img_index, float x, float y, uint32_t color, float scale, float rotation, float opacity)
{
    if(img_index < 0)
    {
        printf("invalid image index!\n");
        return false;
    }

    GFXImage* img = &gfx_images[img_index];

    // int total = img->w*img->h*img->n;
    glUseProgram(program);

    Matrix model = {0};

    Vector2f norm_size = {img->w/1200.0,img->h/800.0};
    Vector2f norm_pos  = {2.0*x/1200.0,2.0*y/800.0};
    Vector2f img_pos = {norm_size.x/2.0 -1.0,norm_size.y/2.0 - 1.0};

    Vector3f pos = {img_pos.x+norm_pos.x,img_pos.y+norm_pos.y,0.0};//x/1200.0,y/800.0,0.0};
    Vector3f rot = {0.0,0.0,rotation};
    Vector3f sca = {scale*norm_size.x,scale*norm_size.y, 1.0};

    get_model_transform(&pos,&rot,&sca,&model);

    //print_matrix(&proj_matrix);
    //print_matrix(&model);

    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color >> 0;

    glUniform3f(location_tint_color,r/255.0,g/255.0,b/255.0);
    glUniform1f(location_opacity,opacity);
    glUniformMatrix4fv(location_model,1,GL_TRUE,&model.m[0][0]);
    glUniformMatrix4fv(location_proj,1,GL_TRUE,&proj_matrix.m[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, img->texture);

    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);

    return true;
}

uint32_t gfx_rgb_to_color(uint8_t r,uint8_t g,uint8_t b)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (r << 0 | g << 8 | b << 16);
#else
    return (r << 24 | g << 16 | b << 8);
#endif
}

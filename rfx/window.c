#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "window.h"

static GLFWwindow* window;

int window_center_x = 0;
int window_center_y = 0;

// int window_width = VIEW_WIDTH;
// int window_height = VIEW_HEIGHT;
// int view_width = VIEW_WIDTH;
// int view_height = VIEW_HEIGHT;
int window_width = 0;
int window_height = 0;
int view_width = 0;
int view_height = 0;

static double window_coord_x = 0;
static double window_coord_y = 0;

static void window_size_callback(GLFWwindow* window, int _window_width, int _window_height);
static void window_maximize_callback(GLFWwindow* window, int maximized);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
static void key_callback(GLFWwindow* window, int key, int scan_code, int action, int mods);
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

bool window_init(int _view_width, int _view_height)
{
    printf("Initializing GLFW.\n");

    if(!glfwInit())
    {
        fprintf(stderr,"Failed to init GLFW!\n");
        return false;
    }


    view_width = _view_width;
    view_height = _view_height;

    window_width = view_width;
    window_height = view_height;

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Want to use OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 

    // printf("vw: %d, vh: %d\n", view_width, view_height);
    window = glfwCreateWindow(view_width,view_height,"Example",NULL,NULL);

    if(window == NULL)
    {
        fprintf(stderr, "Failed to create GLFW Window!\n");
        glfwTerminate();
        return false;
    }

    glfwSetWindowAspectRatio(window,ASPECT_NUM,ASPECT_DEM);
    glfwSetWindowSizeCallback(window,window_size_callback);
    glfwSetWindowMaximizeCallback(window, window_maximize_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    printf("Initializing GLEW.\n");

    // GLEW
    glewExperimental = 1;
    if(glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return false;
    }

    return true;
}

void window_get_mouse_coords(int* x, int* y)
{
    *x = (int)(window_coord_x);
    *y = (int)(window_height - window_coord_y);
}

void window_get_mouse_view_coords(int* x, int* y)
{
    window_get_mouse_coords(x,y);
    *x *= (view_width/(float)window_width);
    *y *= (view_height/(float)window_height);
}

void window_deinit()
{
    glfwTerminate();
}

void window_poll_events()
{
    glfwPollEvents();
}

bool window_should_close()
{
    return (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwWindowShouldClose(window) != 0);
}

void window_swap_buffers()
{
    glfwSwapBuffers(window);
}

static void window_size_callback(GLFWwindow* window, int width, int height)
{
    printf("Window: W %d, H %d\n",width,height);

    window_height = height;
    window_width  = width; //ASPECT_RATIO * window_height;

    int start_x = (window_width + window_width) / 2.0f - window_width;
    int start_y = (window_height + window_height) / 2.0f - window_height;

    window_center_x = window_width / 2;
    window_center_y = window_height / 2;

    glViewport(start_x,start_y,window_width,window_height);
}

static void window_maximize_callback(GLFWwindow* window, int maximized)
{
    if (maximized)
    {
        // The window was maximized
        printf("Maximized.\n");
    }
    else
    {
        // The window was restored
        printf("Restored.\n");
    }
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    window_coord_x = xpos;
    window_coord_y = ypos;
}

typedef struct
{
    uint32_t* keys;
    int key;
    int bit_num;
} WindowKey;

static WindowKey window_keys[32];
static int window_keys_count = 0;

static WindowKey window_mouse_buttons[10];
static int window_mouse_buttons_count = 0;


void window_controls_clear_keys()
{
    memset(window_keys, 0, sizeof(WindowKey)*32);
    memset(window_mouse_buttons, 0, sizeof(WindowKey)*10);
    window_keys_count = 0;
    window_mouse_buttons_count = 0;
}

void window_controls_add_key(uint32_t* keys, int key, int bit_num)
{
    window_keys[window_keys_count].keys = keys;
    window_keys[window_keys_count].key = key;
    window_keys[window_keys_count].bit_num = bit_num;

    window_keys_count++;
}

void window_controls_add_mouse_button(uint32_t* keys, int key, int bit_num)
{
    window_mouse_buttons[window_mouse_buttons_count].keys = keys;
    window_mouse_buttons[window_mouse_buttons_count].key = key;
    window_mouse_buttons[window_mouse_buttons_count].bit_num = bit_num;

    window_mouse_buttons_count++;
}

static void key_callback(GLFWwindow* window, int key, int scan_code, int action, int mods)
{
    bool ctrl = (mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL;

    if(action == GLFW_PRESS)
    {
        switch(key)
        {
            case GLFW_KEY_C:
                if(ctrl)
                {
                    exit(0);
                }
                break;
        }
    }

    if(action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        for(int i = 0; i < window_keys_count; ++i)
        {
            WindowKey* wk = &window_keys[i];
            if(key == wk->key)
            {
                if(action == GLFW_PRESS)
                    (*wk->keys) |= wk->bit_num;
                else
                    (*wk->keys) &= ~(wk->bit_num);
            }
        }
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        for(int i = 0; i < window_mouse_buttons_count; ++i)
        {
            WindowKey* wk = &window_mouse_buttons[i];

            if(button == wk->key)
            {
                if(action == GLFW_PRESS)
                    (*wk->keys) |= wk->bit_num;
                else
                    (*wk->keys) &= ~(wk->bit_num);
            }
        }
    }
}


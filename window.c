#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "settings.h"
#include "window.h"

static GLFWwindow* window;

int window_center_x = 0;
int window_center_y = 0;

int window_width = VIEW_WIDTH;
int window_height = VIEW_HEIGHT;

static double window_coord_x = 0;
static double window_coord_y = 0;

static void window_size_callback(GLFWwindow* window, int _window_width, int _window_height);
static void window_maximize_callback(GLFWwindow* window, int maximized);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
static void key_callback(GLFWwindow* window, int key, int scan_code, int action, int mods);
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

bool window_init()
{
    printf("Initializing GLFW.\n");

    if(!glfwInit())
    {
        fprintf(stderr,"Failed to init GLFW!\n");
        return false;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Want to use OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 

    window = glfwCreateWindow(window_width,window_height,"Example",NULL,NULL);

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

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
    }
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

    /*
    if(action == GLFW_PRESS)
    {
        switch(key)
        {
            case GLFW_KEY_W:
                break;
            case GLFW_KEY_S:
                player.back = true;
                break;
            case GLFW_KEY_A:
                player.left = true;
                break;
            case GLFW_KEY_D:
                player.right = true;
                break;
            case GLFW_KEY_SPACE:
                player.jump = true;
                break;
            case GLFW_KEY_LEFT_SHIFT:
                player.run = true;
                break;
            case GLFW_KEY_ESCAPE:
            {
                int mode = glfwGetInputMode(window,GLFW_CURSOR);
                if(mode == GLFW_CURSOR_DISABLED)
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                else
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }   break;
        }
    }
    else if(action == GLFW_RELEASE)
    {
        switch(key)
        {
            case GLFW_KEY_W:
                player.forward = false;
                break;
            case GLFW_KEY_S:
                player.back = false;
                break;
            case GLFW_KEY_A:
                player.left = false;
                break;
            case GLFW_KEY_D:
                player.right = false;
                break;
            case GLFW_KEY_SPACE:
                player.jump = false;
                break;
            case GLFW_KEY_LEFT_SHIFT:
                player.run = false;
                break;
        }
    }
    */
}

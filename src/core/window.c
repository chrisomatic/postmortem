#include "headers.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "imgui.h"
#include "window.h"

typedef struct
{
    int action;
    int action_prior;
} MouseAction;

static MouseAction mouse_left;
static MouseAction mouse_right;

static GLFWcursor* cursor_ibeam;
static GLFWwindow* window;
static GLFWmonitor* monitor;

int window_width = 0;
int window_height = 0;
int view_width = 0;
int view_height = 0;

static key_cb_t key_cb = NULL;
static KeyMode key_mode = KEY_MODE_NONE;
static char* text_buf = NULL;
static int text_buf_max = 0;

static double window_coord_x = 0;
static double window_coord_y = 0;

static bool get_window_monitor(GLFWmonitor** monitor, GLFWwindow* window);

static void window_size_callback(GLFWwindow* window, int _window_width, int _window_height);
static void window_move_callback(GLFWwindow* window, int xpos, int ypos);
static void window_maximize_callback(GLFWwindow* window, int maximized);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
static void char_callback(GLFWwindow* window, unsigned int code);
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

    glfwWindowHint(GLFW_SAMPLES, 1); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 

    // printf("vw: %d, vh: %d\n", view_width, view_height);
    // window = glfwCreateWindow(view_width,view_height,"Postmortem",glfwGetPrimaryMonitor(),NULL);
    window = glfwCreateWindow(view_width,view_height,"Postmortem",NULL,NULL);

    if(window == NULL)
    {
        fprintf(stderr, "Failed to create GLFW Window!\n");
        glfwTerminate();
        return false;
    }

    glfwSetWindowAspectRatio(window,ASPECT_NUM,ASPECT_DEM);
    glfwSetWindowSizeCallback(window,window_size_callback);
    glfwSetWindowPosCallback(window,window_move_callback);
    glfwSetWindowMaximizeCallback(window, window_maximize_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, char_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwMaximizeWindow(window); //TEMP

    cursor_ibeam = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // vsync
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    printf("Initializing GLEW.\n");

    // GLEW
    glewExperimental = 1;
    if(glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return false;
    }

    mouse_left.action = GLFW_RELEASE;
    mouse_right.action = GLFW_RELEASE;


    monitor = glfwGetPrimaryMonitor();
    // bool ret = get_window_monitor(&monitor, window);
    // if(ret)
    // {
    //     const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    //     printf("Monitor refresh rate: %d (%s)\n", mode->refreshRate, glfwGetMonitorName(monitor));
    // }


    return true;
}


void window_get_mouse_coords(int* x, int* y)
{
    *x = (int)(window_coord_x);
    *y = (int)(window_coord_y);
}

void window_get_mouse_view_coords(int* x, int* y)
{
    window_get_mouse_coords(x,y);

    *x *= (view_width/(float)window_width);
    *y *= (view_height/(float)window_height);
}

void window_set_mouse_view_coords(int x, int y)
{
    double _x = (double)x / (view_width/(float)window_width);
    double _y = (double)y / (view_height/(float)window_height);
    glfwSetCursorPos(window, _x, _y);
}

void window_get_mouse_world_coords(int* x, int* y)
{
    window_get_mouse_view_coords(x,y);
    window_translate_view_to_world(x,y);
}

void window_set_mouse_world_coords(float x, float y)
{
    Matrix* view = get_camera_transform();
    float cam_x = view->m[0][3];
    float cam_y = view->m[1][3];
    double _x = x + cam_x;
    double _y = y + cam_y;
    _x = (double)_x / (view_width/(float)window_width);
    _y = (double)_y / (view_height/(float)window_height);
    // printf("setting mouse world: %.2f, %.2f\n", x, y);
    glfwSetCursorPos(window, _x, _y);
}

void window_deinit()
{
    glfwTerminate();
}

float window_scale_view_to_world(float distance)
{
    float cam_z = camera_get_zoom();
    float factor = 1.0 - cam_z;
    return (distance * factor);
}

void window_translate_view_to_world(int* x, int* y)
{
    Rect r;
    get_camera_rect(&r);

    float cam_z = camera_get_zoom();
    float factor = 1.0 - cam_z;

    Vector2i top_left = {r.x - r.w/2.0, r.y - r.h/2.0};

    int view_x = *x;
    int view_y = *y;

    *x = top_left.x + (factor * view_x);
    *y = top_left.y + (factor * view_y);
}

void window_translate_world_to_view(int* x, int* y)
{
    Rect r;
    get_camera_rect(&r);

    float cam_z = camera_get_zoom();
    float factor = 1.0 - cam_z;

    Vector2i top_left = {r.x - r.w/2.0, r.y - r.h/2.0};

    int world_x = *x;
    int world_y = *y;

    *x = (world_x - top_left.x) / factor;
    *y = (world_y - top_left.y) / factor;
}

void window_poll_events()
{
    glfwPollEvents();
}

bool window_should_close()
{
    return (glfwWindowShouldClose(window) != 0);
}

void window_set_close(int value)
{
    glfwSetWindowShouldClose(window,value);
}

void window_swap_buffers()
{
    glfwSwapBuffers(window);
}

// https://github.com/glfw/glfw/issues/1699
static bool get_window_monitor(GLFWmonitor** monitor, GLFWwindow* window)
{
    bool success = false;

    int window_rectangle[4] = {0};
    glfwGetWindowPos(window, &window_rectangle[0], &window_rectangle[1]);
    glfwGetWindowSize(window, &window_rectangle[2], &window_rectangle[3]);

    int monitors_size = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitors_size);

    GLFWmonitor* closest_monitor = NULL;
    int max_overlap_area = 0;

    for(int i = 0; i < monitors_size; ++i)
    {
        int r[4] = {0};
        glfwGetMonitorWorkarea(monitors[i], &r[0], &r[1], &r[2], &r[3]);

        printf("%d) %s | x,y: %d, %d | w,h: %d, %d\n", i, glfwGetMonitorName(monitors[i]), r[0], r[1], r[2], r[3]);

        int p[2] = {0};
        glfwGetMonitorPos(monitors[i], &p[0], &p[1]);
        printf("%d, %d\n", p[0], p[1]);
    
    }


    for (int i = 0; i < monitors_size; ++i)
    {
        // int monitor_position[2] = {0};
        // glfwGetMonitorPos(monitors[i], &monitor_position[0], &monitor_position[1]);

        const GLFWvidmode* monitor_video_mode = glfwGetVideoMode(monitors[i]);

        int monitor_rectangle[4] = {0};
        glfwGetMonitorWorkarea(monitors[i], &monitor_rectangle[0], &monitor_rectangle[1], &monitor_rectangle[2], &monitor_rectangle[3]);

        // printf("checking monitor %s\n", glfwGetMonitorName(monitors[i]));

        if (
            !(
                ((window_rectangle[0] + window_rectangle[2]) < monitor_rectangle[0]) ||
                (window_rectangle[0] > (monitor_rectangle[0] + monitor_rectangle[2])) ||
                ((window_rectangle[1] + window_rectangle[3]) < monitor_rectangle[1]) ||
                (window_rectangle[1] > (monitor_rectangle[1] + monitor_rectangle[3]))
            )
        ) {
            int intersection_rectangle[4] = {0};

            // x, width
            if (window_rectangle[0] < monitor_rectangle[0])
            {
                intersection_rectangle[0] = monitor_rectangle[0];

                if ((window_rectangle[0] + window_rectangle[2]) < (monitor_rectangle[0] + monitor_rectangle[2]))
                {
                    intersection_rectangle[2] = (window_rectangle[0] + window_rectangle[2]) - intersection_rectangle[0];
                }
                else
                {
                    intersection_rectangle[2] = monitor_rectangle[2];
                }
            }
            else
            {
                intersection_rectangle[0] = window_rectangle[0];

                if ((monitor_rectangle[0] + monitor_rectangle[2]) < (window_rectangle[0] + window_rectangle[2]))
                {
                    intersection_rectangle[2] = (monitor_rectangle[0] + monitor_rectangle[2]) - intersection_rectangle[0];
                }
                else
                {
                    intersection_rectangle[2] = window_rectangle[2];
                }
            }

            // y, height
            if (window_rectangle[1] < monitor_rectangle[1])
            {
                intersection_rectangle[1] = monitor_rectangle[1];

                if ((window_rectangle[1] + window_rectangle[3]) < (monitor_rectangle[1] + monitor_rectangle[3]))
                {
                    intersection_rectangle[3] = (window_rectangle[1] + window_rectangle[3]) - intersection_rectangle[1];
                }
                else
                {
                    intersection_rectangle[3] = monitor_rectangle[3];
                }
            }
            else
            {
                intersection_rectangle[1] = window_rectangle[1];

                if ((monitor_rectangle[1] + monitor_rectangle[3]) < (window_rectangle[1] + window_rectangle[3]))
                {
                    intersection_rectangle[3] = (monitor_rectangle[1] + monitor_rectangle[3]) - intersection_rectangle[1];
                }
                else
                {
                    intersection_rectangle[3] = window_rectangle[3];
                }
            }

            int overlap_area = intersection_rectangle[3] * intersection_rectangle[4];
            if (overlap_area > max_overlap_area)
            {
                closest_monitor = monitors[i];
                // printf("closest monitor %d\n",i);
                max_overlap_area = overlap_area;
            }
        }
    }

    if (closest_monitor)
    {
        *monitor = closest_monitor;
        success = true;
    }

    // true: monitor contains the monitor the window is most on
    // false: monitor is unmodified
    return success;
}

static void window_size_callback(GLFWwindow* window, int width, int height)
{
    printf("Window Resized: W %d, H %d\n",width,height);

    window_height = height;
    window_width  = width; //ASPECT_RATIO * window_height;

    int start_x = 0.0;
    int start_y = 0.0;

    glViewport(start_x,start_y,window_width,window_height);
}

static void window_move_callback(GLFWwindow* window, int xpos, int ypos)
{
    printf("Window Moved: %d, %d\n", xpos, ypos);

    // GLFWmonitor* monitor_prior = monitor;
    // bool ret = get_window_monitor(&monitor, window);
    // if(ret)
    // {
    //     if(monitor != monitor_prior)
    //     {
    //         const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    //         printf("Monitor refresh rate: %d (%s)\n", mode->refreshRate, glfwGetMonitorName(monitor));
    //     }
    // }
}


static void window_maximize_callback(GLFWwindow* window, int maximized)
{
    if(maximized)
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
    int key;
    bool* state;
} WindowKey;

static WindowKey window_keys[32];
static int window_keys_count = 0;

static WindowKey window_mouse_buttons[10];
static int window_mouse_buttons_count = 0;


bool window_controls_is_key_state(int key, int state)
{
    return glfwGetKey(window, key) == state;
}

void window_controls_set_cb(key_cb_t cb)
{
    key_cb = cb;
}

void window_controls_set_text_buf(char* buf, int max_len)
{
    text_buf = buf;
    text_buf_max = max_len;
}

void window_controls_set_key_mode(KeyMode mode)
{
    key_mode = mode;
}

KeyMode window_controls_get_key_mode()
{
    return key_mode;
}


void window_controls_clear_keys()
{
    memset(window_keys, 0, sizeof(WindowKey)*32);
    memset(window_mouse_buttons, 0, sizeof(WindowKey)*10);
    window_keys_count = 0;
    window_mouse_buttons_count = 0;
}

void window_controls_add_key(bool* state, int key)
{
    window_keys[window_keys_count].state = state;
    window_keys[window_keys_count].key = key;

    window_keys_count++;
}

void window_controls_add_mouse_button(bool* state, int key)
{
    window_mouse_buttons[window_mouse_buttons_count].state = state;
    window_mouse_buttons[window_mouse_buttons_count].key = key;

    window_mouse_buttons_count++;
}

bool window_is_cursor_enabled()
{
    int mode = glfwGetInputMode(window,GLFW_CURSOR);
    return (mode == GLFW_CURSOR_NORMAL);
}

void window_enable_cursor()
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void window_disable_cursor()
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

static void char_callback(GLFWwindow* window, unsigned int code)
{
    if(key_mode == KEY_MODE_TEXT)
    {
        char c = (char)code;
        int index = imgui_get_text_cursor_index();
        windows_text_mode_buf_insert(c,index);
        imgui_text_cursor_inc(1);
    }
}

static void key_callback(GLFWwindow* window, int key, int scan_code, int action, int mods)
{
    // printf("key callback: %d\n", key);

    if(key_mode == KEY_MODE_NORMAL)
    {
        if(action == GLFW_PRESS || action == GLFW_RELEASE)
        {
            for(int i = 0; i < window_keys_count; ++i)
            {
                WindowKey* wk = &window_keys[i];
                if(key == wk->key)
                {
                    if(action == GLFW_PRESS)
                        (*wk->state) = true;
                    else
                        (*wk->state) = false;
                }
            }
        }
    }
    else if(key_mode == KEY_MODE_TEXT)
    {
        if(action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            if(key == GLFW_KEY_ENTER)
            {
                windows_text_mode_buf_insert('\n',-1);
            }
            else if(key == GLFW_KEY_BACKSPACE)
            {
                int i0, i1;
                imgui_get_text_cursor_indices(&i0, &i1);

                if(i0 == i1)
                {
                    window_text_mode_buf_remove(i0,true);
                    imgui_text_cursor_inc(-1);
                }
                else
                {
                    for(int i = i0; i < i1; ++i)
                        window_text_mode_buf_remove(i0+1,true);

                    imgui_set_text_cursor_indices(i0,i0);
                }
            }
            else if(key == GLFW_KEY_DELETE)
            {
                int i0, i1;
                imgui_get_text_cursor_indices(&i0, &i1);

                if(i0 == i1)
                {
                    window_text_mode_buf_remove(i0,false);
                }
                else
                {
                    for(int i = i0; i < i1; ++i)
                        window_text_mode_buf_remove(i0+1,true);

                    imgui_set_text_cursor_indices(i0,i0);
                }
            }
            else if(key == GLFW_KEY_LEFT)
            {
                imgui_text_cursor_inc(-1);
            }
            else if(key == GLFW_KEY_RIGHT)
            {
                imgui_text_cursor_inc(+1);
            }
            else if(key == GLFW_KEY_ESCAPE)
            {
                imgui_deselect_text_box();
            }
        }
    }

    if(key_mode != KEY_MODE_NONE && key_cb != NULL)
    {
        key_cb(window, key, scan_code, action, mods);
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
        mouse_left.action_prior = mouse_left.action;
        mouse_left.action = action;
    }
    else if(button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        mouse_right.action_prior = mouse_left.action;
        mouse_right.action = action;
    }

    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        imgui_deselect_text_box();
    }
    
    if(action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        for(int i = 0; i < window_mouse_buttons_count; ++i)
        {
            WindowKey* wk = &window_mouse_buttons[i];

            if(button == wk->key)
            {
                if(action == GLFW_PRESS)
                {
                    (*wk->state) = true;
                    // if(window_is_cursor_enabled())
                    // {
                    //     window_disable_cursor();
                    // }
                }
                else
                    (*wk->state) = false;
            }
        }
    }
}

void window_mouse_update_actions()
{
    mouse_left.action_prior = mouse_left.action;
    mouse_right.action_prior = mouse_right.action;
}

bool window_mouse_left_went_down()
{
    bool went_down_this_frame = (mouse_left.action_prior == GLFW_RELEASE && mouse_left.action == GLFW_PRESS);
    return went_down_this_frame;
}

bool window_mouse_left_went_up()
{
    bool went_up_this_frame = (mouse_left.action_prior == GLFW_PRESS && mouse_left.action == GLFW_RELEASE);
    return went_up_this_frame;
}

void window_mouse_set_cursor_ibeam()
{
    glfwSetCursor(window,cursor_ibeam);
}

void window_mouse_set_cursor_normal()
{
    glfwSetCursor(window,NULL); // standard
}

void windows_text_mode_buf_insert(char c, int index)
{
    if(text_buf != NULL)
    {
        int len = strlen(text_buf);

        if(index == -1)
        {
            index = MIN(len,text_buf_max-1);
        }
        else
        {
            for(int i = len; i >= index; i--)
            {
                if(i+1 >= text_buf_max)
                    continue;

                text_buf[i+1] = text_buf[i];
            }
        }

        text_buf[index] = c;
    }
}

void window_text_mode_buf_remove(int index, bool backspace)
{
    if(text_buf != NULL)
    {
        int len = strlen(text_buf);
        if(len == 0)
            return;

        if(index == -1)
        {
            text_buf[len-1] = '\0';
        }
        else
        {
            if(backspace)
                index-=1;

            if(index < 0)
                return;

            //printf("Help me! index: %d, len: %d\n",index,len);
            for(int i = index; i < len-1; ++i)
            {
                text_buf[i] = text_buf[i+1];
            }
            if(index <= len-1)
                text_buf[len-1] = '\0';
        }
    }
}



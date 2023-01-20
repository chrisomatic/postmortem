#include "headers.h"
#include "math2d.h"
#include "gfx.h"
#include "window.h"
#include "hash.h"
#include "imgui.h"
#include <math.h>

#define NOMINAL_FONT_SIZE 64.0 // px for 1.0 scale
#define MAX_CONTEXTS 32
#define MAX_INT_LOOKUPS 256
#define MAX_HORIZONTAL_STACK 10
#define MAX_TOOLTIP_LEN 256

#define DRAW_DEBUG_BOXES 0

typedef struct
{
    bool used;
    uint32_t hash;
    int val;
} IntLookup;

typedef struct
{
    bool text_click_held;

    int text_cursor_index_held_from;
    int text_cursor_index;

    float text_cursor_x_held_from;
    float text_cursor_x;

    bool highlighted;
} TextBoxProps;

typedef struct
{
    bool draw_on_top;
    uint32_t hash;
    int selected_index;
    char* options;
    int num_options;
    char label[32];
    float text_size_px;
    Rect size;
} DropdownProps;

typedef struct
{
    uint32_t hash;

    uint32_t highlighted_id;
    uint32_t active_id;
    uint32_t focused_text_id;

    // formatting
    Rect curr;

    int start_x, start_y;
    int panel_width, panel_height;
    int max_width;
    int accum_width;
    int horiontal_max_height;

    int indent_amount;

    bool horizontal_stack[MAX_HORIZONTAL_STACK];
    bool is_horizontal;

    IntLookup int_lookups[MAX_INT_LOOKUPS];

    int prior_mouse_x, prior_mouse_y;
    int mouse_x, mouse_y;

    TextBoxProps text_box_props;

    bool has_tooltip;
    char tooltip[MAX_TOOLTIP_LEN+1];
    uint32_t tooltip_hash;

    DropdownProps dropdown_props;

    bool theme_initialized;

} ImGuiContext;

typedef struct
{
    // general
    int spacing;

    // text
    float text_size_px;
    uint32_t text_color;
    float text_scale;
    int text_padding;

    // button
    uint32_t button_color_background;
    uint32_t button_color_background_highlighted;
    uint32_t button_color_background_active;
    uint32_t button_color_foreground;
    float button_opacity;

    // checkbox
    int checkbox_size;

    // slider
    uint32_t slider_color_foreground;
    uint32_t slider_color_background;
    uint32_t slider_color_handle;
    uint32_t slider_color_handle_highlighted;
    uint32_t slider_color_handle_active;
    float slider_width;
    float slider_handle_width;
    float slider_opacity;

    // number box
    int number_box_width;

    // text_box
    uint32_t text_box_color_background;
    uint32_t text_box_color_highlighted;
    int text_box_width;
    int text_box_height;

    // panel
    uint32_t panel_color;
    float panel_opacity;
    int panel_min_width;
    int panel_spacing;

} ImGuiTheme;

static ImGuiContext contexts[MAX_CONTEXTS] = {0};
static ImGuiContext default_context = {0};
static ImGuiTheme theme;
static ImGuiTheme stored_theme;

static ImGuiContext* ctx;

static inline bool is_mouse_inside(int x, int y, int width, int height);
static inline bool is_highlighted(uint32_t hash);
static inline bool is_active(uint32_t hash);
static inline bool is_inside(Rect* r);
static inline void set_highlighted(uint32_t hash);
static inline void set_active(uint32_t hash);
static inline void clear_highlighted();
static inline void clear_active();
static void assign_context(uint32_t hash);
static void handle_highlighting(uint32_t hash, Rect* r);
static void handle_tooltip();
static bool dropdown_on_top(uint32_t hash, Rect* r);
static void imgui_slider_float_internal(char* label, float min, float max, float* result, char* format);
static IntLookup* get_int_lookup(uint32_t hash);
static void progress_pos();
static void mask_off_hidden(char* label, char* new_label, int max_size);
static void set_default_theme();

static void draw_button(uint32_t hash, char* str, Rect* r);
static void draw_image_button(uint32_t hash, char* str, Rect* r, int img_index, int sprite_index,float scale);
static void draw_toggle_button(uint32_t hash, char* str, Rect* r, bool toggled);
static void draw_checkbox(uint32_t hash, char* label, bool result);
static void draw_slider(uint32_t hash, char* str, int slider_x, char* val_format, float val);
static void draw_color_box(Rect* r, uint32_t color);
static void draw_label(int x, int y, uint32_t color, char* label);
static void draw_number_box(uint32_t hash, char* label, Rect* r, int val, int max);
static void draw_input_box(uint32_t hash, char* label, Rect* r, char* text);
static void draw_dropdown(uint32_t hash, char* str, char* options[], int num_options, Rect* r);
static void draw_panel();
static void draw_tooltip();

void imgui_begin(char* name, int x, int y)
{
    uint32_t hash = hash_str(name,strlen(name),0x0);
    assign_context(hash);

    ctx->start_x = x;
    ctx->start_y = y;
    
    ctx->curr.x = x;
    ctx->curr.y = y;

    ctx->indent_amount = 0;
    ctx->max_width = 0;
    ctx->accum_width = 0;
    ctx->horiontal_max_height = 0;

    memset(ctx->horizontal_stack,false,sizeof(bool)*MAX_HORIZONTAL_STACK);

    ctx->text_box_props.highlighted = false;
    ctx->has_tooltip = false;
    ctx->tooltip_hash = 0x0;

    if(!ctx->theme_initialized)
    {
        set_default_theme();
        ctx->theme_initialized = true;
    }

    ctx->prior_mouse_x = ctx->mouse_x;
    ctx->prior_mouse_y = ctx->mouse_y;

    window_get_mouse_view_coords(&ctx->mouse_x, &ctx->mouse_y);
}

void imgui_begin_panel(char* name, int x, int y)
{
    imgui_begin(name, x,y);
    draw_panel();
    ctx->curr.x += theme.panel_spacing;
}

void imgui_set_text_size(float pxsize)
{
    theme.text_size_px = pxsize;
    theme.text_scale = theme.text_size_px / NOMINAL_FONT_SIZE;
}

void imgui_set_text_color(uint32_t color)
{
    theme.text_color = color;
}

void imgui_set_text_padding(int padding)
{
    theme.text_padding = padding;
}

void imgui_set_spacing(int spacing)
{
    theme.spacing = spacing;
}

void imgui_store_theme()
{
    memcpy(&stored_theme, &theme, sizeof(ImGuiTheme));
}

void imgui_restore_theme()
{
    memcpy(&theme, &stored_theme, sizeof(ImGuiTheme));
}

void imgui_set_slider_width(int width)
{
    theme.slider_width = width;
}

void imgui_text(char* text, ...)
{
    va_list args;
    va_start(args, text);
    char str[256] = {0};
    vsprintf(str,text, args);
    va_end(args);

    imgui_text_colored(theme.text_color, str);
}

void imgui_text_sized(float pxsize, char* text, ...)
{
    va_list args;
    va_start(args, text);
    char str[256] = {0};
    vsprintf(str,text, args);
    va_end(args);

    Vector2f size = gfx_draw_string(ctx->curr.x, ctx->curr.y, theme.text_color, (pxsize / NOMINAL_FONT_SIZE), 0.0, 1.0, false, false, str);
    ctx->curr.w = size.x+1.0*theme.spacing;
    ctx->curr.h = 1.3*size.y;

    progress_pos();
}

void imgui_text_colored(uint32_t color, char* text, ...)
{
    va_list args;
    va_start(args, text);
    char str[256] = {0};
    vsprintf(str,text, args);
    va_end(args);

    Vector2f size = gfx_draw_string(ctx->curr.x, ctx->curr.y, color, theme.text_scale, 0.0, 1.0, false, false, str);
    ctx->curr.w = size.x+1.0*theme.spacing;
    ctx->curr.h = 1.3*size.y;

    progress_pos();
}

void imgui_indent_begin(int indentpx)
{
    ctx->indent_amount = indentpx;
    ctx->curr.x += indentpx;
}

void imgui_indent_end()
{
    ctx->curr.x -= ctx->indent_amount;
    ctx->indent_amount = 0;
}

void imgui_newline()
{
    ctx->curr.y += theme.text_size_px;
}

void imgui_horizontal_line()
{
    float width = ctx->panel_width - theme.spacing - 2.0*theme.text_padding;
    gfx_draw_rect_xywh(ctx->curr.x + width/2.0, ctx->curr.y, width,2, theme.text_color, 0.0, 1.0, 1.0, true,false);

    ctx->curr.y += (2+theme.text_padding);
}

void imgui_horizontal_begin()
{
    for(int i = 0; i < MAX_HORIZONTAL_STACK; ++i)
    {
        if(ctx->horizontal_stack[i] == false)
        {
            ctx->horizontal_stack[i] = true;
            ctx->is_horizontal = true;
            break;
        }
    }
}

void imgui_horizontal_end()
{
    for(int i = 0; i < MAX_HORIZONTAL_STACK; ++i)
    {
        if(ctx->horizontal_stack[i] == false)
        {
            if(i > 0)
            {
                ctx->horizontal_stack[i-1] = false;

                if(i == 1)
                {
                    ctx->is_horizontal = false;
                }
                else
                {
                    progress_pos();
                }
            }
            break;
        }
    }

    if(!ctx->is_horizontal)
    {
        ctx->curr.x = ctx->start_x + theme.panel_spacing + ctx->indent_amount;
        ctx->curr.y += ctx->horiontal_max_height;

        ctx->horiontal_max_height = 0;

        ctx->accum_width += ctx->curr.w;
        if(ctx->accum_width + ctx->indent_amount > ctx->max_width)
            ctx->max_width = ctx->accum_width + ctx->indent_amount;

        ctx->accum_width = 0;
    }
}

bool imgui_button(char* label, ...)
{
    bool result = false;
    va_list args;
    va_start(args, label);
    char str[256] = {0};
    vsprintf(str,label, args);
    va_end(args);

    uint32_t hash = hash_str(str,strlen(str),0x0);

    char new_label[256] = {0};
    mask_off_hidden(str, new_label, 256);

    if(is_active(hash))
    {
        if(window_mouse_left_went_up())
        {
            if(is_highlighted(hash))
            {
                result = true;
            }
            clear_active();
        }
    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, new_label);

    Rect interactive = {ctx->curr.x, ctx->curr.y, text_size.x + 2*theme.text_padding, text_size.y + 2*theme.text_padding};
    handle_highlighting(hash, &interactive);

    draw_button(hash, new_label, &interactive);

    ctx->curr.w = text_size.x + 2*theme.text_padding + theme.spacing;
    ctx->curr.h = text_size.y + 2*theme.text_padding + theme.spacing;

    progress_pos();

    return result;
}

bool imgui_image_button(int img_index, int sprite_index, float scale, char* label, ...)
{
    bool result = false;
    va_list args;
    va_start(args, label);
    char str[256] = {0};
    vsprintf(str,label, args);
    va_end(args);

    uint32_t hash = hash_str(str,strlen(str),0x0);

    char new_label[256] = {0};
    mask_off_hidden(str, new_label, 256);

    if(is_active(hash))
    {
        if(window_mouse_left_went_up())
        {
            if(is_highlighted(hash))
            {
                result = true;
            }
            clear_active();
        }
    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, new_label);

    GFXImage* img = &gfx_images[img_index];
    Rect interactive = {ctx->curr.x, ctx->curr.y, img->element_width*scale,img->element_height*scale};
    handle_highlighting(hash, &interactive);

    draw_image_button(hash, new_label, &interactive,img_index,sprite_index,scale);

    ctx->curr.w = interactive.w + 2*theme.text_padding + theme.spacing;
    ctx->curr.h = interactive.h + 2*theme.text_padding + theme.spacing;

    progress_pos();

    return result;
}

void imgui_toggle_button(bool* toggle, char* label, ...)
{
    uint32_t hash = hash_str(label,strlen(label),0x0);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            if(*toggle)
                *toggle = false;
            else
                *toggle = true;
        }
    }

    va_list args;
    va_start(args, label);
    char str[256] = {0};
    vsprintf(str,label, args);
    va_end(args);

    Vector2f text_size = gfx_string_get_size(theme.text_scale, str);

    Rect interactive = {ctx->curr.x, ctx->curr.y, text_size.x + 2*theme.text_padding, text_size.y + 2*theme.text_padding};
    handle_highlighting(hash, &interactive);

    draw_toggle_button(hash, str, &interactive, *toggle);

    ctx->curr.w = text_size.x + 2*theme.text_padding + theme.spacing;
    ctx->curr.h = text_size.y + 2*theme.text_padding + theme.spacing;

    progress_pos();
}

int imgui_button_select(int num_buttons, char* button_labels[], char* label)
{
    if(num_buttons < 0 || num_buttons >= 32)
        return 0;

    char _str[100] = {0};
    snprintf(_str,99,"%s_%s##select%d",label,button_labels[0],num_buttons);

    uint32_t hash = hash_str(_str,strlen(_str),0x0);
    IntLookup* lookup = get_int_lookup(hash);
    int *val = &lookup->val;

    bool results[32] = {false};
    int selection = 0;

    int prior_spacing = theme.spacing;
    imgui_set_spacing(1);
    imgui_horizontal_begin();

    for(int i = 0; i < num_buttons; ++i)
    {
        if(i == *val)
        {
            results[i] = true;
        }

        imgui_toggle_button(&results[i], button_labels[i]);
        if(results[i])
        {
            selection = i;
            *val = i;
        }
    }
    imgui_set_spacing(prior_spacing);
    imgui_text(label);
    imgui_horizontal_end();
    return selection;
}

int imgui_dropdown(char* options[], int num_options, char* label)
{
    if(num_options < 0 || num_options >= 32)
        return 0;

    char _str[100] = {0};
    snprintf(_str,99,"%s_%s##dropdown%d",label,options[0],num_options);

    uint32_t hash = hash_str(_str,strlen(_str),0x0);
    IntLookup* lookup = get_int_lookup(hash);
    int *val = &lookup->val;

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    bool results[32] = {false};
    int selection = 0;

    float max_height = NOMINAL_FONT_SIZE*theme.text_scale + 2*theme.text_padding;
    float max_width = 0.0;

    for(int i = 0; i < num_options; ++i)
    {
        Vector2f text_size = gfx_string_get_size(theme.text_scale, options[i]);
        if(text_size.x > max_width)
        {
            max_width = text_size.x;
        }
    }

    bool active = is_active(hash);

    if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            if(active)
            {
                // make new selection
                float y_diff = ctx->mouse_y - ctx->curr.y;
                int index = floor(y_diff / max_height) - 1;
                if(index >= 0 && index < num_options)
                {
                    ctx->dropdown_props.selected_index = index;
                }
                clear_active();
            }
            else
                set_active(hash);
        }
    }
    else if(is_active(hash))
    {
        if(window_mouse_left_went_down())
        {
            clear_active();
        }
    }

    int display_count = active ? num_options +1 : 1;

    Rect interactive = {ctx->curr.x, ctx->curr.y, max_width+2*theme.text_padding, display_count* (max_height+2*theme.text_padding)};
    handle_highlighting(hash, &interactive);

    if(active)
    {
        ctx->dropdown_props.draw_on_top = true;

        // cache needed properties to draw later
        ctx->dropdown_props.hash = hash;
        ctx->dropdown_props.options = options;
        ctx->dropdown_props.num_options = num_options;
        ctx->dropdown_props.text_size_px = theme.text_size_px;
        memcpy(ctx->dropdown_props.label,new_label,31);
        memcpy(&ctx->dropdown_props.size,&interactive,sizeof(Rect));
    }
    else
    {
        // draw now since the drop down isn't expanded
        ctx->dropdown_props.draw_on_top = false;
        draw_dropdown(hash, new_label, options, num_options, &interactive);
    }

    ctx->curr.w = interactive.w + 2*theme.text_padding + theme.spacing;
    ctx->curr.h = max_height + 4*theme.text_padding + theme.spacing;
    //ctx->curr.h = interactive.h + 2*theme.text_padding + theme.spacing;


    progress_pos();

    return ctx->dropdown_props.selected_index;

}

void imgui_tooltip(char* tooltip, ...)
{
    if(!tooltip)
        return;

    if(is_highlighted(ctx->tooltip_hash))
        return;

    va_list args;
    va_start(args, tooltip);
    char str[256] = {0};
    vsprintf(str,tooltip, args);
    va_end(args);

    memset(ctx->tooltip,0,MAX_TOOLTIP_LEN+1);

    int len = MIN(strlen(str),MAX_TOOLTIP_LEN);
    strncpy(ctx->tooltip,str,len);

    ctx->tooltip_hash = 0x0;
    ctx->has_tooltip = true;
}


void imgui_checkbox(char* label, bool* result)
{
    uint32_t hash = hash_str(label,strlen(label),0x0);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    if(is_active(hash))
    {
        if(window_mouse_left_went_up())
        {
            if(is_highlighted(hash))
            {
                bool curr_result = *result;
                *result = !curr_result;
            }
            clear_active();
        }
    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, label);

    Rect interactive = {ctx->curr.x, ctx->curr.y, theme.checkbox_size + theme.text_padding + text_size.x, theme.checkbox_size};
    handle_highlighting(hash, &interactive);

    draw_checkbox(hash, label, *result);
    progress_pos();
}

void imgui_color_picker(char* label, uint32_t* result)
{
    int r = (*result >> 16) & 0xFF;
    int g = (*result >>  8) & 0xFF;
    int b = (*result >>  0) & 0xFF;

    char lr[32] = {0};
    char lg[32] = {0};
    char lb[32] = {0};

    snprintf(lr,31,"##%s_R",label);
    snprintf(lg,31,"##%s_G",label);
    snprintf(lb,31,"##%s_B",label);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    int prior_spacing = theme.spacing;
    imgui_set_spacing(2);
    imgui_horizontal_begin();
        
        Vector2f s1 = imgui_number_box(lr, 0, 255, &r);
        Vector2f s2 = imgui_number_box(lg, 0, 255, &g);
        Vector2f s3 = imgui_number_box(lb, 0, 255, &b);
        *result = COLOR((uint8_t)r,(uint8_t)g,(uint8_t)b);

        Vector2f text_size = gfx_string_get_size(theme.text_scale, new_label);
        Rect box = {ctx->curr.x,ctx->curr.y, 20, text_size.y+2.0*theme.text_padding};
        draw_color_box(&box,*result);

        draw_label(ctx->curr.x + box.w + theme.text_padding, ctx->curr.y-(text_size.y-box.h)/2.0, theme.text_color, new_label);

        ctx->curr.w = s1.x + s2.x + s3.x + box.w + text_size.x + 2.0*theme.text_padding;
        ctx->curr.h = MAX(text_size.y,box.h);
    imgui_horizontal_end();
    imgui_set_spacing(prior_spacing);
}

void imgui_slider_float(char* label, float min, float max, float* result)
{
    imgui_slider_float_internal(label, min, max, result, "%.4f");
    progress_pos();
}

Vector2f imgui_number_box(char* label, int min, int max, int* result)
{
    uint32_t hash = hash_str(label,strlen(label),0x0);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    IntLookup* lookup = get_int_lookup(hash);
    int *val = &lookup->val;

    if(*result >= min && *result <= max)
        *val = *result;

    if(is_active(hash))
    {
        if(window_mouse_left_went_up())
        {
            clear_active();
        }
        else
        {
            // update val
            *val += (ctx->mouse_x - ctx->prior_mouse_x);
            *val = RANGE(*val, min,max);
        }
    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, new_label);

    Rect interactive = {ctx->curr.x, ctx->curr.y, theme.number_box_width, text_size.y + 2*theme.text_padding};
    handle_highlighting(hash, &interactive);

    draw_number_box(hash, new_label, &interactive, *val,max);

    ctx->curr.w = theme.number_box_width + text_size.x + theme.spacing;
    ctx->curr.h = text_size.y + 2*theme.text_padding + theme.spacing;

    progress_pos();

    *result = *val;

    Vector2f ret = {ctx->curr.w, ctx->curr.h};
    return ret;
}

void imgui_text_box(char* label, char* buf, int bufsize)
{
    uint32_t hash = hash_str(label,strlen(label),0x0);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    float size_array[100] = {0.0};
    int array_size = 0;
    gfx_string_get_size_array(theme.text_scale, &size_array[1],99,&array_size, buf);

    if(ctx->text_box_props.text_cursor_index >= bufsize)
        ctx->text_box_props.text_cursor_index = bufsize-1;

    ctx->text_box_props.text_cursor_x = size_array[ctx->text_box_props.text_cursor_index];
    ctx->text_box_props.text_cursor_x_held_from = size_array[ctx->text_box_props.text_cursor_index_held_from];

    if(is_highlighted(hash))
    {
        ctx->text_box_props.highlighted = true;
        window_mouse_set_cursor_ibeam();

        bool mouse_went_down = window_mouse_left_went_down();

        if(mouse_went_down || ctx->text_box_props.text_click_held)
        {
            float min_dist = 10000.0;
            int selected_index = 0;

            int box_x_start = ctx->curr.x + theme.text_padding;

            if(ctx->mouse_x >= box_x_start)
            {
                int click_x = (int)(ctx->mouse_x - box_x_start);
                for(int i = array_size; i >= 0; --i)
                {
                    float dist = ABS(size_array[i] - (float)click_x);
                    if(dist < min_dist)
                    {
                        min_dist = dist;
                        selected_index = i;
                    }
                }
            }

            ctx->focused_text_id = hash;
            ctx->text_box_props.text_cursor_index = selected_index;
            ctx->text_box_props.text_cursor_x = size_array[selected_index];

            if(mouse_went_down)
            {
                window_controls_set_text_buf(buf,bufsize);
                window_controls_set_key_mode(KEY_MODE_TEXT);
                ctx->text_box_props.text_click_held = true;
                ctx->text_box_props.text_cursor_x_held_from = ctx->text_box_props.text_cursor_x;
                ctx->text_box_props.text_cursor_index_held_from = ctx->text_box_props.text_cursor_index;

            }
        }
    }

    if(ctx->text_box_props.text_click_held && window_mouse_left_went_up())
    {
        ctx->text_box_props.text_click_held = false;
    }

    int str_size = strlen(buf);
    if(ctx->text_box_props.text_cursor_index > str_size)
    {
        ctx->text_box_props.text_cursor_index = str_size;
        ctx->text_box_props.text_cursor_x = size_array[str_size];
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, new_label);

    Rect interactive = {ctx->curr.x, ctx->curr.y, theme.text_box_width, theme.text_box_height};
    handle_highlighting(hash, &interactive);

    draw_input_box(hash, new_label, &interactive, buf);

    ctx->curr.w = theme.text_box_width + text_size.x + theme.text_padding + theme.spacing;
    ctx->curr.h = theme.text_box_height + theme.spacing;

    progress_pos();
}

// demo vars
static int num_clicks = 0;
static uint32_t color1 = 0x00FE2225;
static uint32_t color2 = 0x002468F2;
static char name[20] = {'H','e','l','l','o','\0'};
static char something[20] = {0};
static bool my_check = true;
static int ri = 10;
static bool toggle = false;

Vector2f imgui_draw_demo(int x, int y)
{
    imgui_begin_panel("Demo", x,y);

        imgui_set_text_size(28);
        imgui_text("Demo");
        imgui_set_text_size(12);
        imgui_text_colored(0x00FF00FF, "My name is %s", "Chris");
        imgui_text_colored(0x0000FFFF, "My name is %s", "Kam");

        if(imgui_button("Test Button"))
        {
            num_clicks++;
        }
        imgui_text_colored(0xFFFFFFFF, "Num clicks: %d", num_clicks);

        imgui_button("Dumb Button");
        float v1 = 0.0;
        float v2 = 0.0;
        imgui_slider_float("Slider 1", 0.0,1.0,&v1);
        imgui_slider_float("Slider 2", 0.0,1.0,&v2);

        imgui_checkbox("Checkbox 1",&my_check);

        bool thing;
        imgui_indent_begin(12);
            imgui_checkbox("whatever",&thing);
            imgui_checkbox("dawg",&thing);
        imgui_indent_end();

        imgui_number_box("Some Int##haha",0, 100, &ri);

        imgui_color_picker("Color 1", &color1);
        imgui_color_picker("Color 2", &color2);
        imgui_text_colored(0xFFFFFFFF, "Test");
        imgui_text_box("Name",name,IM_ARRAYSIZE(name));
        imgui_text_box("Something else",something,IM_ARRAYSIZE(something));
        imgui_toggle_button(&toggle, "Toggle me");

        char* buttons[] = {"Apples", "Bananas", "Oranges"};
        int selection = imgui_button_select(3, buttons, "Best Fruit");

        imgui_text_colored(0x00FF00FF,buttons[selection]);

   return imgui_end();
}

static bool _editor_test = false;
static char _editor_text[20]= {0};

void imgui_theme_editor()
{
    float prior_text_size = theme.text_size_px;
    int prior_spacing = theme.spacing;
    imgui_set_text_size(8);
    imgui_set_spacing(2);

    int header_size = 12;
    imgui_text_sized(header_size,"General");
    imgui_indent_begin(10);
    imgui_number_box("Spacing", 0, 100, &theme.spacing);
    imgui_indent_end();

    imgui_text_sized(header_size,"Text");
    imgui_indent_begin(10);
    imgui_slider_float("Text Size", 0.0, 100.0, &theme.text_size_px);
    imgui_color_picker("Text Color", &theme.text_color);
    imgui_number_box("Text Padding", 0, 20, &theme.text_padding);
    imgui_indent_end();

    imgui_text_sized(header_size,"Buttons");
    imgui_indent_begin(10);
    imgui_color_picker("Button Background", &theme.button_color_background);
    imgui_color_picker("Button Foreground", &theme.button_color_foreground);
    imgui_color_picker("Button Highlighted", &theme.button_color_background_highlighted);
    imgui_color_picker("Button Active", &theme.button_color_background_active);
    imgui_slider_float("Button Opacity", 0.0,1.0,&theme.button_opacity);
    imgui_indent_end();

    imgui_text_sized(header_size,"Checkboxes");
    imgui_indent_begin(10);
    imgui_number_box("Checkbox Size", 0, 100, &theme.checkbox_size);
    imgui_checkbox("Test##Checkbox",&_editor_test);
    imgui_indent_end();

    imgui_text_sized(header_size,"Sliders");
    imgui_indent_begin(10);
    imgui_color_picker("Slider Background", &theme.slider_color_background);
    imgui_color_picker("Slider Foreground", &theme.slider_color_foreground);
    imgui_color_picker("Handle Color", &theme.slider_color_handle);
    imgui_color_picker("Handle Highlighted", &theme.slider_color_handle_highlighted);
    imgui_color_picker("Handle Active", &theme.slider_color_handle_active);
    //imgui_slider_float("Slider Width", 0.0,500.0,&theme.slider_width);
    imgui_slider_float("Slider Handle Width", 0.0,50.0,&theme.slider_handle_width);
    imgui_slider_float("Slider Opacity", 0.0,1.0,&theme.slider_opacity);
    imgui_indent_end();

    imgui_text_sized(header_size,"Number Box");
    imgui_indent_begin(10);
    imgui_number_box("Number box width", 0, 100, &theme.number_box_width);
    imgui_indent_end();

    imgui_text_sized(header_size,"Input Text");
    imgui_indent_begin(10);
    imgui_color_picker("text_box Background", &theme.text_box_color_background);
    imgui_color_picker("text_box Highlighted", &theme.text_box_color_highlighted);
    imgui_text_box("Test##text_box",_editor_text,IM_ARRAYSIZE(_editor_text));
    imgui_indent_end();

    imgui_text_sized(header_size,"Panels");
    imgui_indent_begin(10);
    imgui_color_picker("Panel Color", &theme.panel_color);
    imgui_slider_float("Panel Opacity", 0.0,1.0,&theme.panel_opacity);
    imgui_number_box("Panel min width", 0, 1000, &theme.panel_min_width);
    imgui_indent_end();

    imgui_horizontal_begin();
    if(imgui_button("Defaults"))
    {
        set_default_theme();
    }

    imgui_button("Save");
    imgui_horizontal_end();

    imgui_set_text_size(prior_text_size);
    imgui_set_spacing(prior_spacing);
}

void imgui_deselect_text_box()
{
    for(int i = 0; i < MAX_CONTEXTS; ++i)
    {
        contexts[i].focused_text_id = 0x0;
    }

    window_controls_set_key_mode(KEY_MODE_NORMAL);
}

int imgui_get_text_cursor_index()
{
    return ctx->text_box_props.text_cursor_index;
}

void imgui_set_text_cursor_indices(int i0, int i1)
{
    ctx->text_box_props.text_cursor_index = i0;
    ctx->text_box_props.text_cursor_index_held_from = i1;
}

void imgui_get_text_cursor_indices(int* i0, int* i1)
{
    if(ctx->text_box_props.text_cursor_index <= ctx->text_box_props.text_cursor_index_held_from)
    {
        *i0 = ctx->text_box_props.text_cursor_index;
        *i1 = ctx->text_box_props.text_cursor_index_held_from;
    }
    else
    {
        *i0 = ctx->text_box_props.text_cursor_index_held_from;
        *i1 = ctx->text_box_props.text_cursor_index;
    }
}

void imgui_text_cursor_inc(int val)
{
    ctx->text_box_props.text_cursor_index += val;
    ctx->text_box_props.text_cursor_index = MAX(0,ctx->text_box_props.text_cursor_index);
    ctx->text_box_props.text_cursor_index_held_from = ctx->text_box_props.text_cursor_index;
}

Vector2f imgui_end()
{
    ctx->panel_width = MAX(theme.panel_min_width,ctx->max_width+2.0*theme.spacing);
    ctx->panel_height = (ctx->curr.y-ctx->start_y);

    bool text_highlighted = false;
    for(int i = 0; i < MAX_CONTEXTS; ++i)
    {
        if(contexts[i].text_box_props.highlighted)
        {
            text_highlighted = true;
            break;
        }
    }

    if(!text_highlighted)
    {
        window_mouse_set_cursor_normal();
    }

    if(ctx->dropdown_props.draw_on_top && is_active(ctx->dropdown_props.hash))
    {
        imgui_set_text_size(ctx->dropdown_props.text_size_px);
        draw_dropdown(ctx->dropdown_props.hash, ctx->dropdown_props.label, ctx->dropdown_props.options, ctx->dropdown_props.num_options, &ctx->dropdown_props.size);
    }

    if(ctx->has_tooltip && is_highlighted(ctx->tooltip_hash))
    {
        draw_tooltip();
    }

    Vector2f size = {ctx->panel_width, ctx->panel_height};
    return size;
}

// ============================
// Static Functions
// ============================

static inline bool is_mouse_inside(int x, int y, int width, int height)
{
    return (ctx->mouse_x >= x && ctx->mouse_x <= (x+width) &&
                   ctx->mouse_y >= y && ctx->mouse_y <= (y+height));
}
static inline bool is_highlighted(uint32_t hash)
{
    return (ctx->highlighted_id == hash);
}
static inline bool is_active(uint32_t hash)
{
    return (ctx->active_id == hash);
}
static inline bool is_inside(Rect* r)
{
    return is_mouse_inside(r->x, r->y, r->w, r->h);
}
static inline void set_highlighted(uint32_t hash)
{
    ctx->highlighted_id = hash;
}
static inline void set_active(uint32_t hash)
{
    ctx->active_id = hash;
}

static inline void clear_highlighted()
{
    ctx->highlighted_id = 0x0;
}

static inline void clear_active()
{
    ctx->active_id = 0x0;
}

static void set_default_theme()
{
    // set theme
    // text
    theme.spacing = 8;
    theme.text_size_px = 20.0;
    theme.text_scale = theme.text_size_px / NOMINAL_FONT_SIZE;
    theme.text_color = 0xFFFFFFFF;
    theme.text_padding = 4;

    // checkbox
    theme.checkbox_size = 10;

    // button
    theme.button_color_background = 0x55555555;
    theme.button_color_background_highlighted = 0xAAAAAAAA;
    theme.button_color_background_active = 0x0000AAAA;
    theme.button_color_foreground = 0xFFFFFFFF;
    theme.button_opacity = 0.8;

    // slider
    theme.slider_color_foreground = 0xFFFFFFFF;
    theme.slider_color_background = 0x55555555;
    theme.slider_color_handle = 0x8888888;
    theme.slider_color_handle_highlighted = 0xAAAAAAAA;
    theme.slider_color_handle_active = 0x0000AAAA;
    theme.slider_width = 132.0;
    theme.slider_handle_width = 16;
    theme.slider_opacity = 0.8;

    // number box
    theme.number_box_width = 40;

    // text_box
    theme.text_box_color_background = 0x55555555;
    theme.text_box_color_highlighted = 0x66666666;
    theme.text_box_width = 150;
    theme.text_box_height = 22;

    // panel
    theme.panel_color = 0x20202020;
    theme.panel_opacity = 0.85;
    theme.panel_min_width = 200;
    theme.panel_spacing = 8;
}

static void progress_pos()
{

#if DRAW_DEBUG_BOXES
    gfx_draw_rect_xywh(ctx->curr.x + ctx->curr.w/2.0, ctx->curr.y + ctx->curr.h/2.0, ctx->curr.w, ctx->curr.h, 0x0000FFFF, 0.0, 1.0, 1.0, false,false);
#endif

    ctx->accum_width += ctx->curr.w;

    if(ctx->is_horizontal)
    {
        ctx->curr.x += ctx->curr.w;

        if(ctx->curr.h > ctx->horiontal_max_height)
            ctx->horiontal_max_height = ctx->curr.h;
    }
    else
    {
        ctx->curr.y += ctx->curr.h;

        if(ctx->accum_width + ctx->indent_amount > ctx->max_width)
            ctx->max_width = ctx->accum_width + ctx->indent_amount;
        ctx->accum_width = 0;
    }

    ctx->curr.w = 0;
    ctx->curr.h = 0;
}

static void assign_context(uint32_t hash)
{
    for(int i = 0; i < MAX_CONTEXTS;++i)
    {
        if(contexts[i].hash == hash || contexts[i].hash == 0x0)
        {
            ctx = &contexts[i];
            ctx->hash = hash;
            return;
        }
    }
    ctx = &default_context;
}

static void mask_off_hidden(char* label, char* new_label, int max_size)
{
    memcpy(new_label, label, MIN(strlen(label),max_size - 1));

    char* p = new_label;
    bool end = false;
    for(;;)
    {
        if((*p) == '\0') break;
        if(!end && *p == '#' && *(p+1) == '#') end = true;
        if(end) *p = '\0';
        p++;
    }
}

static IntLookup* get_int_lookup(uint32_t hash)
{
    for(int i = 0; i < MAX_INT_LOOKUPS; ++i)
    {
        if(ctx->int_lookups[i].hash == 0x0 || ctx->int_lookups[i].hash == hash)
        {
            IntLookup* lookup = &ctx->int_lookups[i];
            lookup->used = (lookup->hash != 0x0);
            lookup->hash = hash;
            return lookup;
        }
    }
}

static void imgui_slider_float_internal(char* label, float min, float max, float* result, char* format)
{
    if(min > max)
        return;


    uint32_t hash = hash_str(label,strlen(label),0x0);

    char new_label[32] = {0};
    mask_off_hidden(label, new_label, 32);

    Vector2f text_size = gfx_string_get_size(theme.text_scale, new_label);

    // get slider index
    int first_empty = -1;
    int slider_index = -1;

    IntLookup* lookup = get_int_lookup(hash);

    ctx->curr.w = theme.slider_width;
    ctx->curr.h = text_size.y + 2*theme.text_padding;

    float val = 0.0;
    val = *result;
    val -= min;
    val /= (max-min);
    val *= (float)(ctx->curr.w - theme.slider_handle_width);
    lookup->val = roundf(val);

    int *slider_x = &lookup->val;

    if(is_active(hash))
    {
        if(window_mouse_left_went_up())
        {
            clear_active();
        }

        *slider_x = ctx->mouse_x-ctx->curr.x-theme.slider_handle_width/2.0;
        *slider_x = RANGE(*slider_x, 0, ctx->curr.w - theme.slider_handle_width);

    }
    else if(is_highlighted(hash))
    {
        if(window_mouse_left_went_down())
        {
            set_active(hash);
        }
    }

    handle_highlighting(hash, &ctx->curr);

    float slider_val = (*slider_x / (ctx->curr.w - theme.slider_handle_width));
    slider_val *= (max-min);
    slider_val += min;

    draw_slider(hash,new_label, *slider_x, format, slider_val);

    ctx->curr.w = theme.slider_width + text_size.x + 2.0*theme.text_padding;
    ctx->curr.h = text_size.y + 2.0*theme.text_padding + theme.spacing;

    *result = slider_val;
}

static void handle_highlighting(uint32_t hash, Rect* r)
{
    if(is_inside(r) && !dropdown_on_top(hash, r))
    {
        set_highlighted(hash);
    }
    else
    {
        if(is_highlighted(hash))
        {
            clear_highlighted();
        }
    }

    handle_tooltip(hash);
}

static bool dropdown_on_top(uint32_t hash, Rect* r)
{
    if(ctx->dropdown_props.draw_on_top)
    {
        if(ctx->dropdown_props.hash == hash)
        {
            return false;
        }

        if(rectangles_colliding(&ctx->dropdown_props.size,r))
        {
            return true;
        }
    }
    return false;
}

static void handle_tooltip(uint32_t hash)
{
    if(ctx->has_tooltip)
    {
        //if(is_highlighted(hash) && !ctx->tooltip_hash)
        if(!ctx->tooltip_hash)
            ctx->tooltip_hash = hash;
    }
}

static void draw_button(uint32_t hash, char* str, Rect* r)
{
    // draw button
    uint32_t button_color = theme.button_color_background;

    if(is_highlighted(hash))
    {
        button_color = theme.button_color_background_highlighted;
    }
    if(is_active(hash))
    {
        button_color = theme.button_color_background_active;
    }

    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, r->h, button_color, 0.0, 1.0, theme.button_opacity, true,false);

    gfx_draw_string(r->x + theme.text_padding, r->y + theme.text_padding, theme.button_color_foreground, theme.text_scale, 0.0, 1.0, false, false, str);
}

static void draw_image_button(uint32_t hash, char* str, Rect* r, int img_index, int sprite_index,float scale)
{
    // draw button
    uint32_t button_color = theme.button_color_background;

    if(is_highlighted(hash))
    {
        button_color = theme.button_color_background_highlighted;
    }
    if(is_active(hash))
    {
        button_color = theme.button_color_background_active;
    }

    gfx_draw_image_ignore_light(img_index, sprite_index, r->x + r->w/2.0, r->y + r->h/2.0, COLOR_TINT_NONE, scale, 0.0, 1.0, true,false);
    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, r->h, button_color, 0.0, 1.0, 0.3, true,false);

    gfx_draw_string(r->x + r->w + theme.text_padding, r->y, theme.button_color_foreground, theme.text_scale, 0.0, 1.0, false, false, str);
}

static void draw_toggle_button(uint32_t hash, char* str, Rect* r, bool toggled)
{
    // draw button
    uint32_t button_color = theme.button_color_background;

    if(is_highlighted(hash))
    {
        button_color = theme.button_color_background_highlighted;
    }
    if(toggled)
    {
        button_color = theme.button_color_background_active;
    }

    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, r->h, button_color, 0.0, 1.0, theme.button_opacity, true,false);

    gfx_draw_string(r->x + theme.text_padding, r->y + theme.text_padding, theme.button_color_foreground, theme.text_scale, 0.0, 1.0, false, false, str);
}

static void draw_slider(uint32_t hash, char* str, int slider_x, char* val_format, float val)
{
    // draw bar
    gfx_draw_rect_xywh(ctx->curr.x + ctx->curr.w/2.0, ctx->curr.y + ctx->curr.h/2.0, ctx->curr.w, ctx->curr.h, theme.slider_color_background, 0.0, 1.0, theme.slider_opacity, true,false);

    // draw handle
    uint32_t handle_color = theme.slider_color_handle;

    if(is_highlighted(hash))
    {
        handle_color = theme.slider_color_handle_highlighted;
    }
    if(is_active(hash))
    {
        handle_color = theme.slider_color_handle_active;
    }

    gfx_draw_rect_xywh(ctx->curr.x + theme.slider_handle_width/2.0 + slider_x, ctx->curr.y + ctx->curr.h/2.0, theme.slider_handle_width-4, ctx->curr.h-4, handle_color, 0.0, 1.0, theme.slider_opacity, true,false);

    // draw value
    char val_str[16] = {0};
    snprintf(val_str,15,val_format,val);
    Vector2f val_size = gfx_string_get_size(theme.text_scale, val_str);
    gfx_draw_string(ctx->curr.x + (ctx->curr.w-val_size.x)/2.0, ctx->curr.y + theme.text_padding, theme.slider_color_foreground, theme.text_scale, 0.0, 1.0, false, false, val_str);

    // draw label
    gfx_draw_string(ctx->curr.x + ctx->curr.w + theme.text_padding, ctx->curr.y + theme.text_padding, theme.slider_color_foreground, theme.text_scale, 0.0, 1.0, false, false, str);
}

static void draw_panel()
{
    gfx_draw_rect_xywh(ctx->start_x+ctx->panel_width/2.0, ctx->start_y+ctx->panel_height/2.0, ctx->panel_width, ctx->panel_height+theme.spacing, theme.panel_color, 0.0, 1.0, theme.panel_opacity,true,false);
}

static void draw_checkbox(uint32_t hash, char* label, bool result)
{
    // draw button
    uint32_t check_color = theme.button_color_background;

    if(is_highlighted(hash))
    {
        check_color = theme.button_color_background_highlighted;
    }
    if(is_active(hash))
    {
        check_color = theme.button_color_background_active;
    }

    Vector2f text_size = gfx_string_get_size(theme.text_scale, label);

    // draw check
    float x = ctx->curr.x + theme.checkbox_size/2.0;
    float y = ctx->curr.y + (theme.checkbox_size+4)/2.0;

    gfx_draw_rect_xywh(x,y, theme.checkbox_size, theme.checkbox_size, check_color, 0.0, 1.0, 1.0, false,false);
    gfx_draw_rect_xywh(x,y, theme.checkbox_size-4, theme.checkbox_size-4, check_color, 0.0, 1.0, 1.0, result,false);

    gfx_draw_string(ctx->curr.x + theme.checkbox_size + theme.text_padding, ctx->curr.y + (theme.checkbox_size - text_size.y)/2.0, theme.text_color, theme.text_scale, 0.0, 1.0, false, false, label);

    ctx->curr.w = theme.checkbox_size + 2.0*theme.text_padding + text_size.x;
    ctx->curr.h = MAX(theme.checkbox_size, text_size.y)+2.0*theme.text_padding;
}

static void draw_number_box(uint32_t hash, char* label, Rect* r, int val, int max)
{
    uint32_t box_color = theme.button_color_background;

    if(is_highlighted(hash))
    {
        box_color = theme.button_color_background_highlighted;
    }
    if(is_active(hash))
    {
        box_color = theme.button_color_background_active;
    }

    float pct = val/(float)max;

    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, r->h, box_color, 0.0, 1.0, theme.button_opacity, true,false);
    gfx_draw_rect_xywh(r->x + (r->w*pct)/2.0, r->y + r->h/2.0, r->w*pct, r->h, 0x00FFFFFF, 0.0, 1.0, 0.4,true,false);

    char val_str[16] = {0};
    snprintf(val_str,15,"%d",val);
    Vector2f val_size = gfx_string_get_size(theme.text_scale, val_str);

    draw_label(r->x + (r->w - val_size.x)/2.0 , r->y + (r->h-val_size.y)/2.0, theme.button_color_foreground,val_str);
    draw_label(r->x + r->w + theme.text_padding, r->y + (r->h-val_size.y)/2.0, theme.button_color_foreground, label);
}

static void draw_input_box(uint32_t hash, char* label, Rect* r, char* text)
{
    uint32_t box_color = theme.text_box_color_background;

    if(is_highlighted(hash))
    {
        box_color = theme.text_box_color_highlighted;
    }

    Vector2f label_size = gfx_string_get_size(theme.text_scale, label);

    Vector2f text_size = gfx_string_get_size(theme.text_scale, text);

    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, r->h, box_color, 0.0, 1.0, theme.button_opacity, true,false);

    float min_x = ctx->text_box_props.text_cursor_x > ctx->text_box_props.text_cursor_x_held_from ? ctx->text_box_props.text_cursor_x_held_from : ctx->text_box_props.text_cursor_x;
    float sx = r->x + theme.text_padding + min_x; 
    float sy = r->y;
    float sw = ABS(ctx->text_box_props.text_cursor_x_held_from - ctx->text_box_props.text_cursor_x);
    float sh = r->h;

    gfx_draw_rect_xywh(sx + sw/2.0, sy + sh/2.0, sw, sh, 0x000055CC, 0.0, 1.0, theme.button_opacity, true,false);

    gfx_draw_string(r->x+theme.text_padding, r->y-(label_size.y-r->h)/2.0, theme.text_color, theme.text_scale, 0.0, 1.0, false, false, text);

    if(ctx->focused_text_id == hash)
    {
        Vector2f text_size = gfx_string_get_size(theme.text_scale, text);
        //ctx->focused_text_cursor_index;
        float x = r->x+theme.text_padding+ctx->text_box_props.text_cursor_x;
        float y = r->y-(text_size.y-r->h)/2.0f;

        gfx_draw_rect_xywh(x,y+(text_size.y*1.3)/2.0, 1, text_size.y*1.3, theme.text_color, 0.0, 1.0, 1.0, true,false);
    }

    draw_label(r->x + r->w+theme.text_padding, r->y-(label_size.y-r->h)/2.0, theme.text_color, label);
}

static void draw_dropdown(uint32_t hash, char* str, char* options[], int num_options, Rect* r)
{
    // draw button
    uint32_t button_color = theme.button_color_background;

    if(is_active(hash))
    {
        button_color = theme.button_color_background_active;
    }
    else if(is_highlighted(hash))
    {
        button_color = theme.button_color_background_highlighted;
    }

    float box_height = NOMINAL_FONT_SIZE*theme.text_scale + 2*theme.text_padding;

    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + box_height/2.0, r->w, box_height, button_color, 0.0, 1.0, theme.button_opacity, true,false);

    int selection = 0;
    if(ctx->dropdown_props.selected_index >= 0 && ctx->dropdown_props.selected_index < num_options)
        selection = ctx->dropdown_props.selected_index;

    char* selected_text = options[selection];
    gfx_draw_string(r->x + theme.text_padding, r->y + theme.text_padding, theme.button_color_foreground, theme.text_scale, 0.0, 1.0, false, false, selected_text);

    if(is_active(hash))
    {
        float y_diff = ctx->mouse_y - r->y;
        int highlighted_index = floor(y_diff / box_height) - 1;

        uint32_t color;

        for(int i = 0; i < num_options; ++i)
        {
            if(is_highlighted(hash) && i == highlighted_index)
            {
                color = theme.button_color_background_highlighted;
            }
            else
            {
                color = theme.button_color_background;
            }

            gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + (i+1.5)*box_height, r->w, box_height, color, 0.0, 1.0, theme.button_opacity, true,false);
            gfx_draw_string(r->x + theme.text_padding, r->y + theme.text_padding + (i+1)*box_height, theme.button_color_foreground, theme.text_scale, 0.0, 1.0, false, false, options[i]);
        }
    }

    // label
    gfx_draw_string(r->x + r->w + theme.text_padding, r->y + theme.text_padding, theme.button_color_foreground, theme.text_scale, 0.0, 1.0, false, false, str);
}

static void draw_label(int x, int y, uint32_t color, char* label)
{
    gfx_draw_string(x, y, color, theme.text_scale, 0.0, 1.0, false, false, label);
}

static void draw_color_box(Rect* r, uint32_t color)
{
    gfx_draw_rect_xywh(r->x + r->w/2.0, r->y + r->h/2.0, r->w, r->h, color, 0.0, 1.0, 1.0, true,false);
}

static void draw_tooltip()
{
    const float scale = 0.18;
    Vector2f text_size = gfx_string_get_size(scale,ctx->tooltip);

    Rect r = {ctx->mouse_x,ctx->mouse_y,text_size.x+2.0*theme.text_padding,text_size.y+2.0*theme.text_padding};

    r.y -= r.h;

    gfx_draw_rect_xywh(r.x + r.w/2.0, r.y + r.h/2.0, r.w, r.h, 0x00323232, 0.0, 1.0, 0.75, true,false);
    gfx_draw_string(r.x+theme.text_padding, r.y-(text_size.y-r.h)/2.0, theme.text_color, scale, 0.0, 1.0, false, false, ctx->tooltip);

}

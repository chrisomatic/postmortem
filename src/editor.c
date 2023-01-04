#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "main.h"
#include "window.h"
#include "gfx.h"
#include "timer.h"
#include "player.h"
#include "world.h"
#include "imgui.h"
#include "camera.h"
#include "lighting.h"
#include "zombie.h"
#include "io.h"
#include "effects.h"
#include "entity.h"
#include "console.h"
#include "editor.h"

bool editor_enabled = true;

// static ParticleSpawner* particle_spawner;
static ParticleSpawner* particle_spawner;
static void randomize_effect(ParticleEffect* effect);
static char particles_file_name[20] = {0};
static int num_zombies = 10;
static bool editor_collapsed = true;
static float camera_z = 0.4;

static char command_text[100] = {0};

void editor_init()
{
    // create particle effect used for editor
    ParticleEffect effect = {
        .life = {3.0,5.0,1.0},
        .scale = {0.2,0.5,-0.05},
        .velocity_x = {-16.0,16.0,0.0},
        .velocity_y = {-16.0,16.0,0.0},
        .opacity = {0.6,1.0,-0.2},
        .angular_vel = {0.0,0.0,0.0},
        .rotation_init_min = 0.0,
        .rotation_init_max = 0.0,
        .color1 = 0x00FF00FF,
        .color2 = 0x00CC0000,
        .color3 = 0x00202020,
        .spawn_radius_min = 0.0,
        .spawn_radius_max = 1.0,
        .spawn_time_min = 0.2,
        .spawn_time_max = 0.5,
        .burst_count_min = 1,
        .burst_count_max = 3,
        .sprite_index = 57,
        .img_index = particles_image,
        .use_sprite = true,
        .blend_additive = false,
    };

    randomize_effect(&effect);

    particle_spawner = particles_spawn_effect(200, 120, &effect, 0, false, true);
}

ParticleSpawner* editor_get_particle_spawner()
{
    return particle_spawner;
}

void editor_draw()
{
    particle_spawner->hidden = true;

    float tl_x = 10.0;
    float tl_y = 10.0;

    imgui_begin_panel("Editor", (int)tl_x, (int)tl_y);

        imgui_store_theme();
        imgui_set_text_size(12);
        imgui_text_sized(28,"Editor");

        if(editor_collapsed)
        {
            if(imgui_button("Expand"))
            {
                editor_collapsed = false;
            }
        }
        else
        {
            if(imgui_button("Collapse"))
            {
                editor_collapsed = true;
            }
        }

        if(!editor_collapsed)
        {
            imgui_newline();

            char* buttons[] = {"Game", "Weapons", "Particles", "Console", "UI Theme"};
            int selection = imgui_button_select(IM_ARRAYSIZE(buttons), buttons, "");

            imgui_newline();
            imgui_text_sized(20,buttons[selection]);
            imgui_newline();

            // temp
            static float v1=0,v2=0;

            Matrix* view = get_camera_transform();

            float curr_pscale = player->scale;
            float slider_pscale = player->scale;

            switch(selection)
            {
                case 0: // game
                    imgui_color_picker("Ambient Color", &ambient_light);
                    imgui_checkbox("Debug Enabled",&debug_enabled);
                    imgui_slider_float("Player Scale", 0.1,10.0,&slider_pscale);
                    imgui_slider_float("Camera Z", -1.0,1.0,&camera_z);
                    camera_zoom(camera_z, false);


                    Player* p2 = &players[2];
                    char* p2_str = "Activate Player 2";
                    if(p2->active)
                    {
                        p2_str = "Deactivate Player 2";
                    }
                    if(imgui_button(p2_str))
                    {
                        if(!p2->active)
                        {
                            printf("activating player\n");
                            player_set_pos(p2, player->phys.pos.x, player->phys.pos.y);
                            player_count++;
                            p2->active = true;
                        }
                        else
                        {
                            player_count--;
                            p2->active = false;
                        }
                    }

                    imgui_checkbox("Invincible",&player->invincible);

                    // imgui_text_sized(18,"Point Lights");
                    // imgui_slider_float("Atten0", 0.00,1.5,&point_lights[player->point_light].attenuation.x);
                    // imgui_slider_float("Atten1", 0.00,0.03,&point_lights[player->point_light].attenuation.y);
                    // imgui_slider_float("Atten2", 0.0,0.0005,&point_lights[player->point_light].attenuation.z);

                    imgui_text_sized(18,"Zombies");
                    imgui_horizontal_begin();
                    imgui_number_box("##num_zombies", 1, 100, &num_zombies);


                    if(imgui_button("Spawn Zombies"))
                    {
                        ZombieSpawn spawn = {0};
                        spawn.model_index = ZOMBIE1;
                        spawn.model_texture = 0;

                        // get spawn range
                        Rect r = {0};
                        get_camera_rect(&r);
                        float x0 = r.x - r.w/2.0;
                        float x1 = r.x + r.w/2.0;
                        float y0 = r.y - r.h/2.0;
                        float y1 = r.y + r.h/2.0;

                        for(int j = 0; j < num_zombies; ++j)
                        {
                            spawn.pos.x = RAND_FLOAT(x0, x1);
                            spawn.pos.y = RAND_FLOAT(y0, y1);
                            spawn.scale = 1.0;
                            zombie_add(&spawn);
                        }
                    }
                    imgui_horizontal_end();

                    imgui_horizontal_begin();

                    if(imgui_button("Kill All"))
                    {
                        zombie_kill_all();
                    }


                    char* zdebug_str = "Debug Selection";
                    if(zombie_debug)
                    {
                        zdebug_str = "Debug All";
                    }
                    if(imgui_button(zdebug_str))
                    {
                        zombie_debug = !zombie_debug;
                    }

                    char* zfreeze_str = "Freeze";
                    if(zombies_idle)
                    {
                        zfreeze_str = "Unfreeze";
                    }
                    if(imgui_button(zfreeze_str))
                    {
                        zombies_idle = !zombies_idle;
                    }

                    char* zpursue_str = "Pursue";
                    if(zombies_pursue)
                    {
                        zpursue_str = "Don't Pursue";
                    }
                    if(imgui_button(zpursue_str))
                    {
                        zombies_pursue = !zombies_pursue;
                    }
                    imgui_horizontal_end();


                    break;
                case 1:
                    imgui_slider_float("Slider 1", 0.0,1.0,&v1);
                    imgui_slider_float("Slider 2", 0.0,1.0,&v2);
                    break;
                case 2:
                    {

                    ParticleEffect* effect = &particle_spawner->effect;
                    particle_spawner->hidden = false;

                    int big = 12;
                    imgui_set_text_size(10);

                    imgui_horizontal_begin();
                    if(imgui_button("Randomize##particle_spawner"))
                    {
                        randomize_effect(effect);
                    }
                    if(imgui_button("Reload Effects##particle_spawner"))
                    {
                        effects_load_all();
                    }

                    imgui_horizontal_end();

                    //imgui_set_slider_width(60);
                    imgui_text_sized(8,"Particle Count: %d",particle_spawner->particle_list->count);
                    imgui_text_sized(big,"Particle Life");
                    imgui_horizontal_begin();
                        imgui_slider_float("Min##life", 0.1,5.0,&effect->life.init_min);
                        imgui_slider_float("Max##life", 0.1,5.0,&effect->life.init_max);
                        effect->life.init_max = (effect->life.init_min > effect->life.init_max ? effect->life.init_min : effect->life.init_max);
                        imgui_slider_float("Rate##life", 0.1,5.0,&effect->life.rate);
                    imgui_horizontal_end();
                    imgui_text_sized(big,"Rotation");
                    imgui_horizontal_begin();
                        imgui_slider_float("Min##rotation", -360.0,360.0,&effect->rotation_init_min);
                        imgui_slider_float("Max##rotation", -360.0,360.0,&effect->rotation_init_max);
                        effect->rotation_init_max = (effect->rotation_init_min > effect->rotation_init_max ? effect->rotation_init_min : effect->rotation_init_max);
                    imgui_horizontal_end();
                    imgui_text_sized(big,"Scale");
                    imgui_horizontal_begin();
                        imgui_slider_float("Min##scale", 0.01,3.00,&effect->scale.init_min);
                        imgui_slider_float("Max##scale", 0.01,3.00,&effect->scale.init_max);
                        effect->scale.init_max = (effect->scale.init_min > effect->scale.init_max ? effect->scale.init_min : effect->scale.init_max);
                        imgui_slider_float("Rate##scale", -1.0,1.0,&effect->scale.rate);
                    imgui_horizontal_end();
                    imgui_text_sized(big,"Velocity X");
                    imgui_horizontal_begin();
                        imgui_slider_float("Min##velx", -300.0,300.0,&effect->velocity_x.init_min);
                        imgui_slider_float("Max##velx", -300.0,300.0,&effect->velocity_x.init_max);
                        effect->velocity_x.init_max = (effect->velocity_x.init_min > effect->velocity_x.init_max ? effect->velocity_x.init_min : effect->velocity_x.init_max);
                        imgui_slider_float("Rate##velx", -300.0,300.0,&effect->velocity_x.rate);
                    imgui_horizontal_end();
                    imgui_text_sized(big,"Velocity Y");
                    imgui_horizontal_begin();
                        imgui_slider_float("Min##vely", -300.0,300.0,&effect->velocity_y.init_min);
                        imgui_slider_float("Max##vely", -300.0,300.0,&effect->velocity_y.init_max);
                        effect->velocity_y.init_max = (effect->velocity_y.init_min > effect->velocity_y.init_max ? effect->velocity_y.init_min : effect->velocity_y.init_max);
                        imgui_slider_float("Rate##vely", -300.0,300.0,&effect->velocity_y.rate);
                    imgui_horizontal_end();
                    imgui_text_sized(big,"Opacity");
                    imgui_horizontal_begin();
                        imgui_slider_float("Min##opacity", 0.01,1.0,&effect->opacity.init_min);
                        imgui_slider_float("Max##opacity", 0.01,1.0,&effect->opacity.init_max);
                        effect->opacity.init_max = (effect->opacity.init_min > effect->opacity.init_max ? effect->opacity.init_min : effect->opacity.init_max);
                        imgui_slider_float("Rate##opacity", -1.0,1.0,&effect->opacity.rate);
                    imgui_horizontal_end();
                    imgui_text_sized(big,"Angular Velocity");
                    imgui_horizontal_begin();
                        imgui_slider_float("Min##angular_vel", -360.0,360.0,&effect->angular_vel.init_min);
                        imgui_slider_float("Max##angular_vel", -360.0,360.0,&effect->angular_vel.init_max);
                        effect->angular_vel.init_max = (effect->angular_vel.init_min > effect->angular_vel.init_max ? effect->angular_vel.init_min : effect->angular_vel.init_max);
                        imgui_slider_float("Rate##angular_vel", -360.0,360.0,&effect->angular_vel.rate);
                    imgui_horizontal_end();
                    imgui_text_sized(big,"Spawn Radius");
                    imgui_horizontal_begin();
                        imgui_slider_float("Min##spawn_radius", 0.0,64.0,&effect->spawn_radius_min);
                        imgui_slider_float("Max##spawn_radius", 0.0,64.0,&effect->spawn_radius_max);
                        effect->spawn_radius_max = (effect->spawn_radius_min > effect->spawn_radius_max ? effect->spawn_radius_min : effect->spawn_radius_max);
                    imgui_horizontal_end();
                    imgui_text_sized(big,"Spawn Time");
                    imgui_horizontal_begin();
                        imgui_slider_float("Min##spawn_time", 0.01,2.0,&effect->spawn_time_min);
                        imgui_slider_float("Max##spawn_time", 0.01,2.0,&effect->spawn_time_max);
                        effect->spawn_time_max = (effect->spawn_time_min > effect->spawn_time_max ? effect->spawn_time_min : effect->spawn_time_max);
                    imgui_horizontal_end();
                    imgui_text_sized(big,"Burst Count");
                    imgui_horizontal_begin();
                        imgui_number_box("Min##burst_count", 1, 20, &effect->burst_count_min);
                        imgui_number_box("Max##burst_count", 1, 20, &effect->burst_count_max);
                        effect->burst_count_max = (effect->burst_count_min > effect->burst_count_max ? effect->burst_count_min : effect->burst_count_max);
                    imgui_horizontal_end();
                    imgui_checkbox("Use Sprite",&effect->use_sprite);
                    if(effect->use_sprite)
                    {
                        imgui_horizontal_begin();
                        imgui_number_box("Img Index##img_index", 0, MAX_GFX_IMAGES-1, &effect->img_index);
                        imgui_number_box("Sprite Index##sprite_index", 0, MAX_GFX_IMAGES-1, &effect->sprite_index);
                        imgui_horizontal_end();
                    }
                    imgui_text_sized(big,"Colors");
                    imgui_horizontal_begin();
                        imgui_color_picker("1##colors", &effect->color1);
                        imgui_color_picker("2##colors", &effect->color2);
                        imgui_color_picker("3##colors", &effect->color3);
                    imgui_horizontal_end();
                    imgui_checkbox("Blend Addtive",&effect->blend_additive);
                    imgui_newline();

                    imgui_horizontal_begin();

                        imgui_text_box("##file_name_particles",particles_file_name,IM_ARRAYSIZE(particles_file_name));

                        char file_path[64]= {0};
                        snprintf(file_path,63,"effects/%s.effect",particles_file_name);

                        if(imgui_button("Save##particles"))
                        {
                            effects_save(file_path, effect);
                        }
                        if(imgui_button("Load##particles"))
                        {
                            ParticleEffect loaded_effect = {0};
                            bool res = effects_load(file_path, &loaded_effect);
                            if(res)
                            {
                                memcpy(effect,&loaded_effect,sizeof(ParticleEffect));
                            }
                        }

                    imgui_horizontal_end();

                    if(io_file_exists(file_path))
                    {
                        imgui_text_colored(0x00CC8800, "File Exists!");
                    }

                    // show preview
                    particles_draw_spawner(particle_spawner, true, false);

                    } break;
                case 3:
                {
                    int len = strlen(command_text);
                    if(command_text[len-1] == '\n')
                    {
                        console_run_command(command_text);
                        memset(command_text,0,100);
                    }

                    imgui_text_box("Command##console",command_text,IM_ARRAYSIZE(command_text));
                    for(int i = 0; i < console_msg_count; ++i)
                    {
                        imgui_text_colored(console_msg[i].color, "%d: %s",i,console_msg[i].msg);
                    }

                } break;
                case 4:
                    imgui_theme_editor();
                    break;
                default:
                    break;
            }

            if(!FEQ(curr_pscale, slider_pscale))
            {
                player_set_scale(player, slider_pscale);
            }

        }

    imgui_restore_theme();
    Vector2f size = imgui_end();

    Rect panel_rect = {0};
    int px = tl_x;
    int py = tl_y;
    window_translate_view_to_world(&px, &py);
    panel_rect.w = window_scale_view_to_world(size.x);
    panel_rect.h = window_scale_view_to_world(size.y);
    panel_rect.x = px + panel_rect.w/2.0;
    panel_rect.y = py + panel_rect.h/2.0;
    Rect mouse_rect = player_get_mouse_rect(player);
    player->click_ready = !rectangles_colliding(&panel_rect, &mouse_rect);
}

static void randomize_effect(ParticleEffect* effect)
{
    effect->life.init_min = RAND_FLOAT(0.1,5.0);
    effect->life.init_max = RAND_FLOAT(0.1,5.0);
    effect->life.rate     = RAND_FLOAT(1.0,1.0);

    effect->rotation_init_min = RAND_FLOAT(-360.0,360.0);
    effect->rotation_init_max = RAND_FLOAT(-360.0,360.0);

    effect->scale.init_min = RAND_FLOAT(0.01,1.0);
    effect->scale.init_max = RAND_FLOAT(0.01,1.0);
    effect->scale.rate     = RAND_FLOAT(-0.5,0.5);

    effect->velocity_x.init_min = RAND_FLOAT(-32.0,32.0);
    effect->velocity_x.init_max = RAND_FLOAT(-32.0,32.0);
    effect->velocity_x.rate     = RAND_FLOAT(-32.0,32.0);

    effect->velocity_y.init_min = RAND_FLOAT(-32.0,32.0);
    effect->velocity_y.init_max = RAND_FLOAT(-32.0,32.0);
    effect->velocity_y.rate     = RAND_FLOAT(-32.0,32.0);

    effect->opacity.init_min = RAND_FLOAT(0.01,1.0);
    effect->opacity.init_max = RAND_FLOAT(0.01,1.0);
    effect->opacity.rate     = RAND_FLOAT(-1.0,1.0);

    effect->angular_vel.init_min = RAND_FLOAT(-360.0, 360.0);
    effect->angular_vel.init_max = RAND_FLOAT(-360.0, 360.0);
    effect->angular_vel.rate     = RAND_FLOAT(-360.0, 360.0);

    effect->spawn_radius_min  = RAND_FLOAT(0.0, 64.0);
    effect->spawn_radius_max  = RAND_FLOAT(0.0, 64.0);

    effect->spawn_time_min  = RAND_FLOAT(0.01, 2.0);
    effect->spawn_time_max  = RAND_FLOAT(0.01, 2.0);

    effect->burst_count_min  = RAND_FLOAT(1, 20);
    effect->burst_count_max  = RAND_FLOAT(1, 20);

    effect->img_index = particles_image;
    effect->sprite_index = RAND_RANGE(0,79);

    effect->color1 = RAND_RANGE(0x0,0x00FFFFFF);
    effect->color2 = RAND_RANGE(0x0,0x00FFFFFF);
    effect->color3 = RAND_RANGE(0x0,0x00FFFFFF);

    effect->blend_additive = RAND_RANGE(0,1) == 1 ? true : false;
}

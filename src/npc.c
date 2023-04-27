#include "headers.h"
#include "math2d.h"
#include "gfx.h"
#include "camera.h"
#include "world.h"
#include "npc.h"

static void npc_init_models()
{

}

static void npc_init_images()
{
    //none
    for(int pm = 0; pm < PLAYER_MODELS_MAX; ++pm)
    {
        for(int t = 0; t < PLAYER_TEXTURES_MAX; ++t)
        {
            for(int ps = 0; ps < ANIM_MAX; ++ps)
            {
                player_image_sets_none[pm][t][ps] = -1;

                char fname[100] = {0};
                sprintf(fname, "src/img/characters/%s_%d-%s.png", player_models[pm].name, t, player_anim_state_str(ps));
                // if(access(fname, F_OK) == 0)
                if(io_file_exists(fname))
                {
                    player_image_sets_none[pm][t][ps] = gfx_load_image(fname, false, true, IMG_ELEMENT_W, IMG_ELEMENT_H);
                    // printf("%s -> %d\n", fname, player_image_sets[pm][t][ps][wt]);
                }
            }
        }
    }

}

void npc_init()
{
    npc_init_models();
    npc_init_images();
}

#include "io.h"
#include "log.h"
#include "effects.h"

ParticleEffect particle_effects[MAX_PARTICLE_EFFECTS];

EffectEntry effect_map[] = {
    {EFFECT_GUN_SMOKE1,"gun_smoke.effect"},
    {EFFECT_SPARKS1,"sparks1.effect"},
    {EFFECT_BLOOD1,"blood1.effect"},
    {EFFECT_DEBRIS1,"debris1.effect"},
    {EFFECT_MELEE1,"melee1.effect"},
    {EFFECT_BULLET_CASING,"bullet_casing.effect"},
    {EFFECT_FIRE,"fire.effect"},
    {EFFECT_BLOCK_DESTROY,"block_destroy.effect"},
    {EFFECT_SMOKE,"smoke.effect"},
    {EFFECT_SMOKE2,"smoke2.effect"},
    {EFFECT_BULLET_TRAIL,"bullet_trail.effect"},
    {EFFECT_GUN_BLAST,"gun_blast.effect"},
};

static int get_effect_map_index(char* file_name)
{
    int num_effects_in_map = sizeof(effect_map)/sizeof(EffectEntry);
    for(int i = 0; i < num_effects_in_map; ++i)
    {
        if(strcmp(effect_map[i].file_name, file_name) == 0)
        {
            return i;
        }
    }
    return -1;
}

void effects_load_all()
{
    char files[32][32] = {0};
    int num_effects = io_get_files_in_dir("effects",".effect", files);

    LOGI("Num effects: %d",num_effects);

    for(int i = 0; i < num_effects; ++i)
    {
        char full_path[64] = {0};
        snprintf(full_path,63,"effects/%s",files[i]);
        printf("files[i]: %s\n",files[i]);
        int index = get_effect_map_index(files[i]);
        if(index == -1)
        {
            LOGW("Failed to map effect %s", files[i]);
        }
        else if(index >= MAX_PARTICLE_EFFECTS)
        {
            LOGW("Map effect is out of max particles range, %d",index);
        }
        else
        {
            effects_load(full_path,&particle_effects[index]);
            LOGI("%d: %s",i, files[i]);
        }
    }
}

bool effects_save(char* file_path, ParticleEffect* effect)
{
    FILE* fp = fopen(file_path,"wb");
    if(!fp)
    {
        LOGW("Failed to save file %s\n",file_path);
        return false;
    }

    fwrite(effect,sizeof(ParticleEffect),1,fp);
    LOGI("Saved effect: %s",file_path);
    fclose(fp);
    return true;
}

bool effects_load(char* file_path, ParticleEffect* effect)
{
    FILE* fp = fopen(file_path,"rb");
    if(!fp)
    {
        LOGW("Failed to load file %s\n",file_path);
        return false;
    }

    fread(effect,sizeof(ParticleEffect),1,fp);
    LOGI("Loaded effect: %s", file_path);
    fclose(fp);
    return true;
}

#include "io.h"
#include "log.h"
#include "effects.h"

ParticleEffect particle_effects[MAX_PARTICLE_EFFECTS];

void effects_load_all()
{
    char files[10][32] = {0};
    int num_effects = io_get_files_in_dir("effects",files);

    printf("Num effects: %d\n",num_effects);

    for(int i = 0; i < num_effects; ++i)
    {
        char full_path[64] = {0};
        snprintf(full_path,63,"effects/%s",files[i]);
        effects_load(full_path,&particle_effects[i]);
        printf("%d: %s\n",i, files[i]);
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

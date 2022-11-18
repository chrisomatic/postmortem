#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#ifdef WIN32
#include <io.h>
#define F_OK 0
#define access _access
#endif

static bool io_file_exists(char* file_path)
{
    return (access(file_path, F_OK) == 0);
}

static int io_get_files_in_dir(char* dir_path, char files[][32])
{
    DIR *dir = opendir(dir_path);

    if(!dir)
        return 0;

    int num_files = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
        if(strcmp(ent->d_name,".") ==  0 || strcmp(ent->d_name,"..") == 0)
            continue;

        strcpy(files[num_files],ent->d_name);
        num_files++;
    }

    closedir(dir);
    return num_files;
}

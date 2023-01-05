#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define access _access
#endif

static bool io_file_exists(char* file_path)
{
    return (access(file_path, F_OK) == 0);
}

static int io_get_files_in_dir(char* dir_path, char* match_str, char files[][32])
{
#ifdef _WIN32

    // const int max_size = MAX_PATH;
    const int max_size = 32;

    WIN32_FIND_DATA ffd;
    TCHAR szDir[max_size];
    HANDLE hFind = INVALID_HANDLE_VALUE;
    size_t length_of_arg;
    int num_files = 0;

    StringCchLength(directory_path, max_size, &length_of_arg);

    if (length_of_arg > (max_size - 3))
    {
        printf(TEXT("\nDirectory path is too long.\n"));
        return (-1);
    }

    StringCchCopy(szDir, max_size, directory_path);
    StringCchCat(szDir, max_size, TEXT("\\*"));
    StringCchCat(szDir, max_size, TEXT(match_str));

    hFind = FindFirstFile(szDir, &ffd);

    if (INVALID_HANDLE_VALUE == hFind)
    {
        printf(TEXT("\nError getting first file.\n"));
        return -1;
    } 

    char full_file_path[max_size] = {0};
    StringCchCopy(full_file_path,max_size,directory_path);
    StringCchCat(full_file_path, max_size, "\\");
    StringCchCat(full_file_path, max_size, ffd.cFileName);
    StringCchCopy(files[num_files],max_size,full_file_path);

    do
    {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // directory
        }
        else
        {
            StringCchCopy(full_file_path,max_size,directory_path);
            StringCchCat(full_file_path, max_size, "\\");
            StringCchCat(full_file_path, max_size, ffd.cFileName);
            StringCchCopy(files[num_files++],max_size,full_file_path);
        }
    } while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);
    return num_files;

#else

    DIR *dir = opendir(dir_path);

    if(!dir)
        return 0;

    int num_files = 0;
    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL)
    {
        if(strstr(ent->d_name,match_str) == NULL || strcmp(ent->d_name,".") ==  0 || strcmp(ent->d_name,"..") == 0)
            continue;

        strcpy(files[num_files],ent->d_name);
        num_files++;
    }

    closedir(dir);
    return num_files;

#endif
}

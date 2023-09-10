#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include "array_list.h"


int comparator(const void* x_void, const void* y_void){
    return strcmp(*(char**)x_void, *(char**)y_void); /*strcmp can do the comparing for qsort*/
}


void print_formatted(char* filename, int level, int s){
    struct stat info;
    off_t size;
    int i = 0;
    for(i = 0; i < level - 1; i++){
        printf("|   ");
    }
    if(s == 1){
        lstat(filename, &info);
        size = info.st_size;
        printf("|-- %s [size: %ld]\n", filename, size);
    }
    else{
        printf("|-- %s\n", filename);
    }
}

void print_parent(int s){
    char* token = NULL;
    char* strcop = NULL;
    struct stat info;
    off_t size;
    char* buffer = getcwd(NULL, 0);
    /*printf("buffer: %s\n", buffer);*/
    token = strtok(buffer, "/");
    
    while(token != NULL){
        free(strcop);
        strcop = strdup(token);
        token = strtok(NULL, "/");
    }
    if(s == 0){
        printf("%s\n", strcop);
    }
    else{
        lstat(strcop, &info);
        size = info.st_size;
        printf("%s [size: %ld]\n", strcop, size);
    }
    free(strcop);
    free(token);
    free(buffer);

}

void free_all_dup_in_arr(char** arr, int size){
    int i = 0;
    for(i = 0; i < size; i++){
        free(arr[i]);
    }
}

void print_dir_recursive(char* opath, char* tpath, int level, int a, int s){ /*level means how deep the current dir is compared to the big parent dir*/
    DIR *d;
    struct dirent *dir;
    struct arraylist* name_list = array_list_new();
    char* prev_path = strdup(opath); 
    char* cur_path = NULL;
    char* tpath_copy = strdup(tpath);
    /*printf("prev path - %s\n", prev_path);*/
    int i = 0, j = 0;
    int ch;
    level++;
    if(tpath_copy  == NULL){
        tpath_copy  = ".";
    }
    
    /*checks if we can change into the directory*/
    d = opendir(tpath_copy ); /*but first we open the target dir*/
    ch = chdir(tpath_copy );/*then we check if the target dir is valid*/

    if(ch < 0){/*if not, then report error, free allocs and exit*/
        printf("invalid filename: %s\n", tpath_copy );
        free_all_dup_in_arr(name_list -> arr, name_list -> num_items);   
        free(name_list -> arr);
        free(name_list);
        chdir(prev_path);
        free(prev_path);
        free(tpath_copy);
        return;
    }
    cur_path = getcwd(NULL, 0);

    print_parent(s);/*valid dir, so we print the name of the dir here*/

    if(d){/*checks if dir has any files/directory*/
        while((dir = readdir(d)) != NULL){
            if(a == 1){
                if(strcmp(dir -> d_name, ".") && strcmp(dir -> d_name, "..")){
    
                    array_list_add_to_end(name_list, strdup(dir -> d_name));/*have to strdup because when we close dir, we free the pointers to d_name*/
                }
            }
            else{
                if((dir -> d_name)[0] != '.' ){/*only add regular file/directory to filename list*/
                    array_list_add_to_end(name_list, strdup(dir -> d_name));
                }
            }
            
        }
        closedir(d);

        
        qsort(name_list -> arr, name_list -> num_items ,sizeof(char*), comparator); /*sorts the filename array*/
        /*printf("list size: %d", name_list -> num_items);*/
        for(i = 0; i <  (name_list -> num_items); i++){/*a for loop to iterate thru the filename array*/
            /*printf("%d: %s\n", i, (name_list -> arr)[i]);*/
            if((d = opendir((name_list -> arr)[i])) != NULL){ /*right here checks if the current filename is a directory*/
                closedir(d);/*closedir right here because we dont need it open*/
                for(j = 0; j < level - 1; j++){/*if it is, then first print the tree format*/
                    printf("|   ");
                }
                printf("|-- ");
                print_dir_recursive(cur_path, (name_list -> arr)[i], level, a, s);/*now begins the recursion*/
                continue;
            }
            print_formatted((name_list -> arr)[i], level, s); 
        }

    }

    
    /*free(cur_dir);*/
    free(dir);
    ch = chdir(prev_path);
    free(cur_path);
    free_all_dup_in_arr(name_list -> arr, name_list -> num_items);      
    free(name_list -> arr);
    free(name_list);
    free(prev_path);
    free(tpath_copy );
}

int process_args(int argc, char** argv, int* a, int* s){ /*returns the index of first arg that is not -a and -s*/
    int i = 1; /* this is for keeping track of current iteration in argv*/
    if(argc > 1){
        while((i < argc) && (*a != 1 || *s != 1)){ /*this loop condition basically is a for loop that also checks if a and s are already true*/
            if(!strcmp(argv[i], "-a")){/*checks if the current arg is -a*/
                *a = 1; /*if so, set a to true (1)*/ 
            }
            else if (!strcmp(argv[i], "-s")) /*basically the same thing for s*/
            {
                *s = 1;
            }
            else{
                return i; /*now we return the index of actual filename arg*/
            }
            i++;
        }
    }
    return i;
}

void tree(int index, int argc, char** argv, char* main_dir, int a, int s){
    if(index >= argc){
        print_dir_recursive(main_dir, NULL, 0, a, s);
    }
    else{
        while(index < argc){
            print_dir_recursive(main_dir, argv[index], 0, a, s);
            index++;
        }
    }
}

int main(int argc, char** argv){
    int a = 0; /*these two int are to indicate whether or not the users typed -a and/or -s*/
    int s = 0; /*0 means false and 1 means true*/
    int index = process_args(argc, argv, &a, &s);
    char* main_dir = getcwd(NULL, 0);
    tree(index, argc, argv, main_dir, a, s);
    free(main_dir);
    return 0;
}

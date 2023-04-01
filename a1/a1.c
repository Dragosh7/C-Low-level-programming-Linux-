#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
void listRec(const char *path,int size_smaller)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;

    dir = opendir(path);
    if(dir == NULL) {
        perror("Could not open directory");
        return;
    }
    printf("SUCCESS\n");
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if(lstat(fullPath, &statbuf) == 0) {
                if(size_smaller < 0 || statbuf.st_size < size_smaller){
                printf("%s\n", fullPath);}
                if(S_ISDIR(statbuf.st_mode)) {
                    listRec(fullPath,size_smaller);
                }
            }
        }
    }
    closedir(dir);
}

void listDir(const char* path, const char* prefix, int size_smaller) {
    // Deschiderea directorului
    DIR* dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "ERROR\ninvalid directory path\n");
        return;
    }
    printf("SUCCESS\n");
    // Citirea continutului directorului
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Ignorarea directoriilor curente si parinte
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // Obtinerea informatiilor despre fisierul/directorul curent
        char fullPath[512];
        snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
        struct stat info;
        if (stat(fullPath, &info) < 0) {
            continue;
        }
        // Verificarea daca fisierul/directorul corespunde criteriilor de cautare
        if (S_ISDIR(info.st_mode)) {
            // Director
             printf("%s/%s\n",prefix, entry->d_name);
         } else if (size_smaller < 0 || info.st_size < size_smaller) {
            // Fisier cu dimensiune mai mica decat limita specificata
            printf("%s/%s\n",prefix, entry->d_name);
        }
    }
    closedir(dir);
}
int main(int argc, char **argv){
    if(argc<2) return 0;

    if(strcmp(argv[1], "variant") == 0)   printf("90958\n");
    
    if(strcmp(argv[1], "list")== 0){
        const char* path = NULL;
        int size = -1;
        short recursiv=0;
        for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "path=", 5) == 0) {
            path = &argv[i][5];

        } else if (strncmp(argv[i], "size_smaller=", 13) == 0) {
            size = atoi(&argv[i][13]);
        } 
          else if (strcmp(argv[i], "recursive") == 0) {  recursiv = 1;} 
          
          else {
            perror("ERROR\ninvalid command\n");
            return -1;
        }
    }
        if (path == NULL) {
        perror("ERROR\ninvalid command\n");
        return -1;
         }
     if(recursiv) {listRec(path,size);}
     else { listDir(path,path,size);}
    }
    
    return 0;
}
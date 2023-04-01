#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

void sf(char* file_path) {
    FILE* fp = fopen(file_path, "rb");
    if (fp == NULL) {
        perror("ERROR\nCannot open file\n");
        return;
    }

    int magic;
    if (fread(&magic, sizeof(int), 1, fp) != 1) {
        perror("ERROR\nError reading magic\n");
        fclose(fp);
        return;
    }
    if (magic != 0) {
        perror("ERROR\nWrong magic\n");
        fclose(fp);
        return;
    }

    int version;
    if (fread(&version, sizeof(int), 1, fp) != 1) {
        perror("ERROR\nError reading version\n");
        fclose(fp);
        return;
    }
    if (version < 36 || version > 74) {
        perror("ERROR\nWrong version\n");
        fclose(fp);
        return;
    }

    int nr_sections;
    if (fread(&nr_sections, sizeof(int), 1, fp) != 1) {
        perror("ERROR\nError reading sections\n");
        fclose(fp);
        return;
    }
    if (nr_sections < 7 || nr_sections > 14) {
        perror("ERROR\nWrong number of sections\n");
        fclose(fp);
        return;
    }

    int section_types[nr_sections];
    if (fread(section_types, sizeof(int), nr_sections, fp) != nr_sections) {
        perror("ERROR\nError reading types\n");
        fclose(fp);
        return;
    }
    int i;
    for (i = 0; i < nr_sections; i++) {
        if (section_types[i] != 68 && section_types[i] != 38 && section_types[i] != 59) {
            perror("ERROR\nWrong types\n");
            fclose(fp);
            return;
        }
    }
  printf("SUCCESS\nversion=%d\nnr_sections=%d\n", version, nr_sections);

    int size, offset;
    char name[10];
    for (i = 0; i < nr_sections; i++) {
        if (fread(name, sizeof(char), 10, fp) != 10) {
            perror("ERROR\nError reading section name\n");
            fclose(fp);
            return;
        }
        if (fread(&size, sizeof(int), 1, fp) != 1) {
            perror("ERROR\nError reading section size\n");
            fclose(fp);
            return;
        }
        if (fread(&offset, sizeof(int), 1, fp) != 1) {
            perror("ERROR\nError reading section offset\n");
            fclose(fp);
            return;
        }
        printf("section%d: %s %d %d\n", i+1, name, section_types[i], size);
    }

    fclose(fp);
}
void listRec(const char *path,int size_smaller,const char* name)
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
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if(lstat(fullPath, &statbuf) == 0) {
                    if (name == NULL || strncmp(entry->d_name, name, strlen(name)) == 0) {
                            if(S_ISDIR(statbuf.st_mode)) {
                                    if(size_smaller<0){printf("%s\n", fullPath);}
                            }
                            else if(size_smaller<0 || statbuf.st_size < size_smaller){printf("%s\n", fullPath);}
                     }
                if(S_ISDIR(statbuf.st_mode)) {
                    listRec(fullPath,size_smaller,name);
                }
            }
        }
    }
    closedir(dir);
}

void listDir(const char* path, int size_smaller,const char* name) {
    DIR* dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "ERROR\ninvalid directory path\n");
        return;
    }
    printf("SUCCESS\n");
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char fullPath[512];
        snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
        struct stat info;
        if (stat(fullPath, &info) < 0) {
            continue;
        }

        if (S_ISDIR(info.st_mode)) {
            if ((name == NULL || strncmp(entry->d_name, name, strlen(name)) == 0) && size_smaller<0) {
             printf("%s/%s\n", path, entry->d_name);}
         } 
            
         else if (size_smaller < 0 || info.st_size < size_smaller) {
            if (name == NULL || strncmp(entry->d_name, name, strlen(name)) == 0) {
                    printf("%s/%s\n", path, entry->d_name);
                }
        }
    }
    closedir(dir);
}
int main(int argc, char **argv){
    if(argc<2) return 0;

    if(strcmp(argv[1], "variant") == 0)   printf("90958\n");
    
    if(strcmp(argv[1], "list")== 0){
        const char* path = NULL;
        const char* name = NULL;
        int size = -1;
        short recursiv=0;
        for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "path=", 5) == 0) {
            path = &argv[i][5];

        } else if (strncmp(argv[i], "size_smaller=", 13) == 0) {
            size = atoi(&argv[i][13]);
        } 
          else if (strcmp(argv[i], "recursive") == 0) {  recursiv = 1;} 

          else if (strncmp(argv[i], "name_starts_with=", 17) == 0) {
            name = &argv[i][17];
        } 
          else {
            perror("ERROR\ninvalid command\n");
            return -1;
        }
    }
        if (path == NULL) {
        perror("ERROR\ninvalid command\n");
        return -1;
         }
     if(recursiv) {printf("SUCCESS\n");listRec(path,size,name);}
     else { listDir(path,size,name);}
    }
    
    if(strcmp(argv[1], "parse")== 0){
        char* path = NULL;
        
        if (strncmp(argv[2], "path=", 5) == 0) {
            path = &argv[2][5];
        } 
          else {
            perror("ERROR\ninvalid command\n");
            return -1;
        }
        if (path == NULL) {
        perror("ERROR\ninvalid command\n");
        return -1;
         }
         sf(path);
     
    }
    return 0;
}
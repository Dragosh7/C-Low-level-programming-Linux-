#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct
{
    unsigned char sect_name[13];
    unsigned short sect_type;
    unsigned int sect_offset;
    unsigned int sect_size;
} SECTION_HEADER;

void sf(char *file_path, int sect_nr, int line_nr)
{
    int fd = -1;

    fd = open(file_path, O_RDONLY);
    if (fd < 0)
    {
        printf("ERROR\ninvalid file\n");
        return;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    unsigned char magic;
    lseek(fd, file_size - 1, SEEK_SET);
    if (read(fd, &magic, 1) < 0)
    {
        perror("ERROR\nError reading magic\n");
        close(fd);
        return;
    }
    if (magic != '0')
    {
        printf("ERROR\nwrong magic\n");
        close(fd);
        return;
    }

    lseek(fd, file_size - 3, SEEK_SET);
    unsigned short header_size;
    if (read(fd, &header_size, 2) < 0)
    {
        perror("ERROR\nError reading header size\n");
        close(fd);
        return;
    }

    lseek(fd, file_size - header_size, SEEK_SET);
    unsigned int version;
    if (read(fd, &version, 4) < 0)
    {
        perror("ERROR\nError reading version\n");
        close(fd);
        return;
    }
    if (version < 36 || version > 74)
    {
        printf("ERROR\nwrong version\n");
        close(fd);
        return;
    }
    lseek(fd, file_size - header_size + 4, SEEK_SET);
    unsigned char nr_ofsections;
    if (read(fd, &nr_ofsections, 1) < 0)
    {
        perror("ERROR\nError reading sections\n");
        close(fd);
        return;
    }
    short nr_sections = nr_ofsections;
    if (nr_sections < 7 || nr_sections > 14)
    {
        printf("ERROR\nwrong sect_nr\n");
        close(fd);
        return;
    }

    SECTION_HEADER *header = malloc(sizeof(SECTION_HEADER) * nr_sections);

    for (int i = 0; i < nr_sections; i++)
    {
        if (read(fd, &header[i].sect_name, 12) < 0)
        {
            perror("ERROR\nError reading section name\n");
            close(fd);
            free(header);
            return;
        }
        if (read(fd, &header[i].sect_type, 2) < 0)
        {
            perror("ERROR\nError reading section type\n");
            close(fd);
            free(header);
            return;
        }
        if (header[i].sect_type != 68 && header[i].sect_type != 38 && header[i].sect_type != 59)
        {
            printf("ERROR\nwrong sect_types\n");
            close(fd);
            free(header);
            return;
        }
        if (read(fd, &header[i].sect_offset, 4) < 0)
        {
            perror("ERROR\nError reading section offset\n");
            close(fd);
            free(header);
            return;
        }
        if (read(fd, &header[i].sect_size, 4) < 0)
        {
            perror("ERROR\nError reading section size\n");
            close(fd);
            free(header);
            return;
        }
    }
    if (sect_nr == 0 && line_nr == 0)
    {
        printf("SUCCESS\nversion=%d\nnr_sections=%d\n", version, nr_sections);
        for (int i = 0; i < nr_sections; i++)
        {
            printf("section%d: %s %d %d\n", i + 1, header[i].sect_name, header[i].sect_type, header[i].sect_size);
        }
    }
    else
    {
        if (sect_nr > nr_ofsections)
        {
            printf("ERROR\ninvalid section\n");
            close(fd);
            free(header);
            return;
        }
        if (line_nr <= 0)
        {
            printf("ERROR\ninvalid line\n");
            close(fd);
            free(header);
            return;
        }
        lseek(fd, header[sect_nr].sect_offset, SEEK_SET);
        char extract[22];
        int num_read, line_num = 1;
        off_t offset = 0;
        while ((num_read = read(fd, extract, 22)) > 0)
        {
            for (int i = 0; i < num_read; i++)
            {
                if (line_num == line_nr)
                {
                    printf("%s", &extract[i]);
                }

                if (extract[i] == '\n')
                {
                    line_num++;
                    if (line_num > line_nr)
                    {
                        close(fd);
                        return;
                    }
                }
            }
            offset += num_read;
            if (lseek(fd, offset, SEEK_SET) == -1)
            {
                perror("lseek");
                return;
            }
        }
    }
    free(header);
    close(fd);
}
void listRec(const char *path, int size_smaller, const char *name)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;

    dir = opendir(path);
    if (dir == NULL)
    {
        perror("Could not open directory");
        return;
    }
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if (lstat(fullPath, &statbuf) == 0)
            {
                if (name == NULL || strncmp(entry->d_name, name, strlen(name)) == 0)
                {
                    if (S_ISDIR(statbuf.st_mode))
                    {
                        if (size_smaller < 0)
                        {
                            printf("%s\n", fullPath);
                        }
                    }
                    else if (size_smaller < 0 || statbuf.st_size < size_smaller)
                    {
                        printf("%s\n", fullPath);
                    }
                }
                if (S_ISDIR(statbuf.st_mode))
                {
                    listRec(fullPath, size_smaller, name);
                }
            }
        }
    }
    closedir(dir);
}

void listDir(const char *path, int size_smaller, const char *name)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        fprintf(stderr, "ERROR\ninvalid directory path\n");
        return;
    }
    printf("SUCCESS\n");

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char fullPath[512];
        snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
        struct stat info;
        if (stat(fullPath, &info) < 0)
        {
            continue;
        }

        if (S_ISDIR(info.st_mode))
        {
            if ((name == NULL || strncmp(entry->d_name, name, strlen(name)) == 0) && size_smaller < 0)
            {
                printf("%s/%s\n", path, entry->d_name);
            }
        }

        else if (size_smaller < 0 || info.st_size < size_smaller)
        {
            if (name == NULL || strncmp(entry->d_name, name, strlen(name)) == 0)
            {
                printf("%s/%s\n", path, entry->d_name);
            }
        }
    }
    closedir(dir);
}
int main(int argc, char **argv)
{
    if (argc < 2)
        return 0;

    if (strcmp(argv[1], "variant") == 0)
        printf("90958\n");

    if (strcmp(argv[1], "list") == 0)
    {
        const char *path = NULL;
        const char *name = NULL;
        int size = -1;
        short recursiv = 0;
        for (int i = 2; i < argc; i++)
        {
            if (strncmp(argv[i], "path=", 5) == 0)
            {
                path = &argv[i][5];
            }
            else if (strncmp(argv[i], "size_smaller=", 13) == 0)
            {
                size = atoi(&argv[i][13]);
            }
            else if (strcmp(argv[i], "recursive") == 0)
            {
                recursiv = 1;
            }

            else if (strncmp(argv[i], "name_starts_with=", 17) == 0)
            {
                name = &argv[i][17];
            }
            else
            {
                perror("ERROR\ninvalid command\n");
                return -1;
            }
        }
        if (path == NULL)
        {
            perror("ERROR\ninvalid command\n");
            return -1;
        }
        if (recursiv)
        {
            printf("SUCCESS\n");
            listRec(path, size, name);
        }
        else
        {
            listDir(path, size, name);
        }
    }

    if (strcmp(argv[1], "parse") == 0)
    {
        char *path = NULL;

        if (strncmp(argv[2], "path=", 5) == 0)
        {
            path = &argv[2][5];
        }
        else
        {
            perror("ERROR\ninvalid command\n");
            return -1;
        }
        if (path == NULL)
        {
            perror("ERROR\ninvalid command\n");
            return -1;
        }

        sf(path, 0, 0);
    }
    if (strcmp(argv[1], "extract") == 0)
    {
        char *path = NULL;
        int section = 0;
        int line = 0;

        for (int i = 2; i < argc; i++)
        {
            if (strncmp(argv[i], "path=", 5) == 0)
            {
                path = &argv[i][5];
            }
            else if (strncmp(argv[i], "section=", 8) == 0)
            {
                section = atoi(&argv[i][8]);
            }
            else if (strncmp(argv[i], "line=", 5) == 0)
            {
                line = atoi(&argv[i][5]);
            }
            else
            {
                perror("ERROR\ninvalid command\n");
                return -1;
            }
        }
        if (path == NULL)
        {
            perror("ERROR\ninvalid command\n");
            return -1;
        }
        sf(path, section, line);
    }

    return 0;
}
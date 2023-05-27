#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define PIPE_REQ "REQ_PIPE_90958"
#define PIPE_RESP "RESP_PIPE_90958"

unsigned int convertHex(const char *buffer, int start_position)
{
    unsigned int number = 0;
    for (int i = 0; i < sizeof(unsigned int); i++)
    {
        number |= (unsigned int)(buffer[start_position + i] & 0xFF) << (8 * i);
    }
    return number;
}

int main()
{
    int fd_wr = -1;
    int fd_r = -1;
    int shm = -1;
    unsigned char *shm_data = NULL;
    unsigned char *file_data = NULL;
    unsigned int shm_size = 0;
    unsigned int file_size = 0;
    if (mkfifo(PIPE_RESP, 0600) != 0)
    {
        printf("ERROR\ncannot create the response pipe\n");
        return 1;
    }

    fd_r = open(PIPE_REQ, O_RDONLY);
    if (fd_r == -1)
    {
        unlink(PIPE_RESP);
        close(fd_r);
        printf("ERROR\ncannot open the request pipe\n");
        return 1;
    }

    fd_wr = open(PIPE_RESP, O_WRONLY);
    if (fd_wr == -1)
    {
        unlink(PIPE_RESP);
        unlink(PIPE_REQ);
        close(fd_r);
        close(fd_wr);
        printf("ERROR\ncannot open the response pipe\n");
        return 1;
    }

    char msg[] = "START#";
    if (write(fd_wr, msg, strlen(msg)) != -1)
    {
        printf("SUCCESS\n");
    }

    while (1)
    {
        char buffer[251];
        int buf_len = 0;
        // int buf_len = read(fd_r,&buffer,250);
        for (int i = 0; i < 250; i++)
        {
            buf_len = i;
            read(fd_r, &buffer[i], 1);
            if (buffer[i] == '#')
                break;
        }
        buffer[++buf_len] = '\0';

        if (strcmp(buffer, "ECHO#\0") == 0)
        {
            unsigned int variant = 90958;

            write(fd_wr, "ECHO#", 5);
            write(fd_wr, "VARIANT#", 8);
            write(fd_wr, &variant, sizeof(variant));
        }

        if (strncmp(buffer, "CREATE_SHM", 10) == 0)
        {
            // int shm;
            read(fd_r, &shm_size, sizeof(shm_size)); // convertHex(buffer,10);//4432204;

            shm = shm_open("/yfjm11bg", O_CREAT | O_RDWR, 0644);

            if (shm < 0 || ftruncate(shm, shm_size) == -1)
            {

                write(fd_wr, "CREATE_SHM#", 11);
                write(fd_wr, "ERROR#", 6);
                shm_unlink("/yfjm11bg");
                munmap(shm_data, shm_size);
            }

            shm_data = (unsigned char *)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

            if (shm_data != MAP_FAILED)
            {
                write(fd_wr, "CREATE_SHM#", 11);
                write(fd_wr, "SUCCESS#", 8);
            }
            else
            {
                write(fd_wr, "CREATE_SHM#", 11);
                write(fd_wr, "ERROR#", 6);
                shm_unlink("/yfjm11bg");
                munmap(shm_data, shm_size);
            }
        }

        if (strncmp(buffer, "WRITE_TO_SHM", 12) == 0)
        {
            unsigned int offset = 0;
            unsigned int value = 0;
            read(fd_r, &offset, sizeof(offset));
            read(fd_r, &value, sizeof(value));

            if (offset < 0 || offset > shm_size - 4)
            {
                write(fd_wr, "WRITE_TO_SHM#", 13);
                write(fd_wr, "ERROR#", 6);
            }
            else
            {
                *(unsigned int *)(shm_data + offset) = value;
                write(fd_wr, "WRITE_TO_SHM#", 13);
                write(fd_wr, "SUCCESS#", 8);
            }
        }

        if (strncmp(buffer, "MAP_FILE", 8) == 0)
        {

            unsigned int len = 0;
            int fd = -1;
            char path[251];
            // read(fd_r, path, 250);
            // path[251] = '\0';

            for (int i = 0; i < 250; i++)
            {
                len = i;
                read(fd_r, &path[i], 1);
                if (path[i] == '#')
                    break;
            }
            path[++len] = '\0';
            write(fd_wr, "MAP_FILE#", 9);

            fd = open(path, O_RDONLY);

            file_size = lseek(fd, 0, SEEK_END);
            lseek(fd, 0, SEEK_SET);

            file_data = (unsigned char *)mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);

            if (file_data == (void *)-1)
            {
                write(fd_wr, "ERROR#", 6);
            }
            else
            {
                write(fd_wr, "SUCCESS#", 8);
            }
        }

        if (strcmp(buffer, "EXIT#\0") == 0)
        {
            // close(fd_r);
            // close(fd_wr);
            // unlink(PIPE_RESP);
            // unlink(PIPE_REQ);
            // return 1;
            break;
        }
    }

    shm_unlink("/yfjm11bg");
    munmap(shm_data, shm_size);
    munmap(file_data, file_size);
    close(fd_r);
    close(fd_wr);
    unlink(PIPE_RESP);
    unlink(PIPE_REQ);
    return 0;
}
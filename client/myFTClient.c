#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void read_file(int socket, const char *file_path);

void write_file(int socket, const char *file_path);

void list_directory(int socket, const char *dir_path);

int main(int argc, char **argv)
{
    printf("num args: %d\n", argc);

    switch (argv[1][1])
    {
    case 'l':
        printf("list\n");
        break;
    case 'r':
        printf("read\n");
        break;
    case 'w':
        printf("write\n");
        break;
    default:
        printf("Invalid command.\n");
        break;
    }
}
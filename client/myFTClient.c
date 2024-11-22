#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int connect_to_server(const char *server_address, int server_port);

void read_file(int socket, const char *file_path);

void write_file(int socket, const char *file_path);

void list_directory(char **argv);

int main(int argc, char **argv)
{
    printf("num args: %d\n", argc);

    switch (argv[1][1])
    {
    case 'l':
        if (argc == 8)
        {
            list_directory(argv);
        }
        else
        {
            printf("Invalid number of arguments.\n");
        }
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
    return 0;
}

int connect_to_server(const char *server_address, int server_port)
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        perror("Socket creation failed.");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_address, &server_addr.sin_addr);

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Connection failed.");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    return socket_fd;
}

void list_directory(char **argv)
{
    char *server_address;
    int server_port;
    char *remote_directory;
    printf("list_directory\n");
    if (strcmp(argv[2], "-a") != 0 || strcmp(argv[4], "-p") != 0 || strcmp(argv[6], "-f") != 0)
    {
        printf("Invalid arguments.\n");
        return;
    }
    server_address = argv[3];
    printf("server_address: %s\n", server_address);
    server_port = atoi(argv[5]);
    printf("server_port: %d\n", server_port);
    remote_directory = argv[7];
    printf("remote_directory: %s\n", remote_directory);

    int socket = connect_to_server(server_address, server_port);

    // list files in remote directory
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "LIST %s", remote_directory);
    send(socket, buffer, strlen(buffer), 0);

    ssize_t bytes_received;
    while ((bytes_received = recv(socket, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    close(socket);
}

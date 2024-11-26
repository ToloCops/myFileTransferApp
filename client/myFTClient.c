#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int connect_to_server(const char *server_address, int server_port);

void read_file(int argc, char **argv);

void write_file(int argc, char **argv);

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
        if (argc == 8 || argc == 10)
        {
            read_file(argc, argv);
        }
        else
        {
            printf("Invalid number of arguments.\n");
        }
        break;
    case 'w':
        if (argc == 8 || argc == 10)
        {
            write_file(argc, argv);
        }
        else
        {
            printf("Invalid number of arguments.\n");
        }
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

void read_file(int argc, char **argv)
{
    char *server_address;
    int server_port;
    char *remote_file_path;
    char *local_file_path;
    printf("read_file\n");
    if (strcmp(argv[2], "-a") != 0 || strcmp(argv[4], "-p") != 0 || strcmp(argv[6], "-f") != 0)
    {
        printf("Invalid arguments.\n");
        return;
    }
    server_address = argv[3];
    printf("server_address: %s\n", server_address);
    server_port = atoi(argv[5]);
    printf("server_port: %d\n", server_port);
    remote_file_path = argv[7];
    printf("remote_file_path: %s\n", remote_file_path);
    if (argc == 10)
    {
        if (strcmp(argv[8], "-o") != 0)
        {
            printf("Invalid arguments.\n");
            return;
        }
        local_file_path = argv[9];
        printf("local_file_path: %s\n", local_file_path);
    }
    else
    {
        local_file_path = remote_file_path;
        printf("local_file_path: %s\n", local_file_path);
    }

    int socket = connect_to_server(server_address, server_port);

    // send command to server
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "READ %s", remote_file_path);
    send(socket, buffer, strlen(buffer), 0);

    FILE *file = fopen(local_file_path, "w");
    if (file == NULL)
    {
        perror("File opening failed.");
        close(socket);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = recv(socket, buffer, BUFFER_SIZE, 0)) > 0)
    {
        if (strncmp(buffer, "ERR:", 4) == 0)
        {
            printf("Error: %s\n", buffer + 4);
            fclose(file);
            close(socket);
            return;
        }
        fwrite(buffer, 1, bytes_read, file);
    }

    fclose(file);
    close(socket);
}

void write_file(int argc, char **argv)
{
    char *server_address;
    int server_port;
    char *local_file_path;
    char *remote_file_path;
    printf("write_file\n");
    if (strcmp(argv[2], "-a") != 0 || strcmp(argv[4], "-p") != 0 || strcmp(argv[6], "-f") != 0)
    {
        printf("Invalid arguments.\n");
        return;
    }
    server_address = argv[3];
    printf("server_address: %s\n", server_address);
    server_port = atoi(argv[5]);
    printf("server_port: %d\n", server_port);
    local_file_path = argv[7];
    printf("local_file_path: %s\n", local_file_path);
    if (argc == 10)
    {
        if (strcmp(argv[8], "-o") != 0)
        {
            printf("Invalid arguments.\n");
            return;
        }
        remote_file_path = argv[9];
        printf("remote_file_path: %s\n", remote_file_path);
    }
    else
    {
        remote_file_path = local_file_path;
        printf("remote_file_path: %s\n", remote_file_path);
    }

    int socket = connect_to_server(server_address, server_port);

    // send file to server
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "WRITE %s", remote_file_path);
    send(socket, buffer, strlen(buffer), 0);

    // Attendi la conferma dal server
    ssize_t bytes_received = recv(socket, buffer, BUFFER_SIZE, 0);
    printf("response: %s\n", buffer);
    if (bytes_received <= 0 || strncmp(buffer, "OK:", 3) != 0)
    {
        printf("Failed to receive confirmation from server.\n");
        close(socket);
        return;
    }

    FILE *file = fopen(local_file_path, "r");
    if (file == NULL)
    {
        perror("File opening failed.");
        close(socket);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        send(socket, buffer, bytes_read, 0);
    }

    fclose(file);
    close(socket);
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

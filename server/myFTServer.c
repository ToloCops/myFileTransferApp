#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

typedef struct
{
    int socket;
    char *root_directory;
} ClientData;

void *handle_client(void *arg);

void setup_directory(const char *dir_path);

int main(int argc, char *argv[])
{
    if (argc != 7)
    {
        fprintf(
            stderr,
            "Usage: %s -a server_address -p server_port -d ft_root_directory\n");
        exit(EXIT_FAILURE);
    }

    char *server_address = NULL;
    int server_port = 0;
    char *root_directory = NULL;

    // Parsing arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-a") == 0)
        {
            server_address = argv[++i];
        }
        else if (strcmp(argv[i], "-p") == 0)
        {
            server_port = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            root_directory = argv[++i];
        }
    }

    if (server_address == NULL || server_port == 0 || root_directory == NULL)
    {
        fprintf(stderr, "Missing arguments.\n");
        exit(EXIT_FAILURE);
    }

    setup_directory(root_directory);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        perror("Socket creation failed.");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_address, &server_addr.sin_addr);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
        -1)
    {
        perror("Binding failed.");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server_fd, 10) == -1)
    {
        perror("Listen failed.");

        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening at %s:%d\n", server_address, server_port);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                               &client_addr_len);

        if (client_fd == -1)
        {
            perror("Accept failed.");
            continue;
        }

        printf("Client connected.\n");

        ClientData *client_data = malloc(sizeof(ClientData));
        client_data->socket = client_fd;
        client_data->root_directory = root_directory;

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client_data);
        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}

void *handle_client(void *arg)
{
    ClientData *client_data = (ClientData *)arg;
    int client_fd = client_data->socket;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[bytes_received] = '\0';
        printf("Comando ricevuto: %s\n", buffer);
        // Qui va aggiunta la gestione dei vari comandi
    }

    close(client_fd);
    free(client_data);
    return NULL;
}

void setup_directory(const char *dir_path)
{
    if (mkdir(dir_path, 0700) == -1 && errno != EEXIST)
    {
        perror("Directory creation failed.\n");
        exit(EXIT_FAILURE);
    }
}
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define BUFFER_SIZE 1024

typedef struct
{
    int socket;
    char *root_directory;
} ClientData;

void *handle_client(void *arg);

void setup_directory(const char *dir_path);

void get_file_list(const char *dir_path, const char *root, int client_fd);

void write_file(const char *dir_path, const char *root, int client_fd);

void read_file(const char *dir_path, const char *root, int client_fd);

int main(int argc, char *argv[])
{
    // Checking number of arguments
    if (argc != 7)
    {
        fprintf(
            stderr,
            "Usage: %s -a server_address -p server_port -d ft_root_directory\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Server variables
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

    // Checking if all arguments are set
    if (server_address == NULL || server_port == 0 || root_directory == NULL)
    {
        fprintf(stderr, "Missing arguments.\n");
        exit(EXIT_FAILURE);
    }

    // Create the root directory
    setup_directory(root_directory);

    // Create the server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Check if the socket was created successfully
    if (server_fd == -1)
    {
        perror("Socket creation failed.");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_address, &server_addr.sin_addr);

    // Bind the socket to the server address
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

    // Print the server information
    printf("Server listening at %s:%d\n", server_address, server_port);

    // Try to accept incoming connections
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

        // Set up the client data structure
        ClientData *client_data = malloc(sizeof(ClientData));
        client_data->socket = client_fd;
        client_data->root_directory = root_directory;

        // Create a new thread to handle the client
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client_data);
        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}

void get_file_list(const char *rel_path, const char *root, int client_fd)
{
    // Incrementa il puntatore per saltare eventuali spazi all'inizio del percorso
    while (*rel_path == ' ')
        rel_path++;

    // Costruisce il percorso completo
    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s/%s", root, rel_path);

    // Controlla che il percorso sia all'interno della directory root
    char real_path[BUFFER_SIZE];
    if (realpath(full_path, real_path) == NULL || strncmp(real_path, root, strlen(root)) != 0)
    {
        const char *error_msg = "ERR: Access to directory not allowed\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
        return;
    }

    // Apre la directory
    DIR *dir = opendir(real_path);
    if (dir == NULL)
    {
        const char *error_msg = "ERR: Unable to open directory\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
        return;
    }

    // Prepara la lista dei file
    char file_list[BUFFER_SIZE] = "";
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            strcat(file_list, entry->d_name);
            strcat(file_list, "\n");
        }
    }
    closedir(dir);

    // Controlla se la directory è vuota
    if (strlen(file_list) == 0)
    {
        strcpy(file_list, "No files found in the directory.\n");
    }

    // Invia la lista dei file al client
    send(client_fd, file_list, strlen(file_list), 0);
}

void write_file(const char *dir_path, const char *root, int client_fd)
{
    // Skip any spaces at the beginning of the path
    while (*dir_path == ' ')
        dir_path++;

    // Build the full path
    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s/%s", root, dir_path);

    // Extract the directory path
    char *last_slash = strrchr(full_path, '/');
    if (last_slash != NULL)
    {
        *last_slash = '\0'; // Termina la stringa alla directory
    }

    // Check if the path is within the root directory
    char real_path[BUFFER_SIZE];
    if (realpath(full_path, real_path) == NULL || strncmp(real_path, root, strlen(root)) != 0)
    {
        const char *error_msg = "ERR: Access to file not allowed\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
        return;
    }

    // Restore the slash
    if (last_slash != NULL)
    {
        *last_slash = '/';
    }

    const char *confirm_msg = "OK: Ready to receive file\n";
    send(client_fd, confirm_msg, strlen(confirm_msg), 0);

    // Open the file for writing
    FILE *file = fopen(full_path, "wb");
    if (file == NULL)
    {
        perror("Failed to open file");
        const char *error_msg = "ERR: Unable to open file\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Receive data from the client
    while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        if (fwrite(buffer, 1, bytes_received, file) != bytes_received)
        {
            perror("Failed to write to file");
            fclose(file);
            const char *error_msg = "ERR: Failed to write file\n";
            send(client_fd, error_msg, strlen(error_msg), 0);
            return;
        }
    }

    fclose(file);

    const char *success_msg = "OK: File written successfully\n";
    send(client_fd, success_msg, strlen(success_msg), 0);

    return;
}

void read_file(const char *dir_path, const char *root, int client_fd)
{
    // Skip any spaces at the beginning of the path
    while (*dir_path == ' ')
        dir_path++;

    // Build the full path
    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s/%s", root, dir_path);

    // Check if the path is within the root directory
    char real_path[BUFFER_SIZE];
    if (realpath(full_path, real_path) == NULL || strncmp(real_path, root, strlen(root)) != 0)
    {
        const char *error_msg = "ERR: Access to file not allowed\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
        return;
    }

    // Open the file for reading
    FILE *file = fopen(full_path, "rb");
    if (file == NULL)
    {
        perror("Failed to open file");
        const char *error_msg = "ERR: Unable to open file\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Send the file data to the client
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        send(client_fd, buffer, bytes_read, 0);
    }

    fclose(file);

    return;
}

void *handle_client(void *arg)
{
    ClientData *client_data = (ClientData *)arg;
    int client_fd = client_data->socket;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Receive data from the client
    while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        // Null-terminate the received data
        buffer[bytes_received] = '\0';
        printf("Comando ricevuto: %s\n", buffer);

        // Check the command
        if (strncmp(buffer, "LIST", 4) == 0)
        {
            char *relative_path = buffer + 4;
            get_file_list(relative_path, client_data->root_directory, client_fd);

            close(client_fd);
            free(client_data);
            return NULL;
        }
        else if (strncmp(buffer, "WRITE", 5) == 0)
        {
            write_file(buffer + 5, client_data->root_directory, client_fd);

            close(client_fd);
            free(client_data);
            return NULL;
        }
        else if (strncmp(buffer, "READ", 4) == 0)
        {
            read_file(buffer + 4, client_data->root_directory, client_fd);

            close(client_fd);
            free(client_data);
            return NULL;
        }
        else
        {
            const char *error_msg = "ERR: Command not recognized\n";
            send(client_fd, error_msg, strlen(error_msg), 0);
        }
    }
    // Close the client socket and free the client data
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

struct Client {
    int socket;
    char username[50];
    struct Client* next;
};

struct Client* head = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void send_message_to_clients_custom(struct Client* sender, const char* message) {
    pthread_mutex_lock(&mutex);
    struct Client* current = head;
    while (current != NULL) {
        if (current != sender) {
            char message_with_sender[100];
            snprintf(message_with_sender, sizeof(message_with_sender), "[%s]: %s", sender->username, message);
            send(current->socket, message_with_sender, strlen(message_with_sender), 0);
        }
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
}

void remove_client_custom(struct Client* client) {
    pthread_mutex_lock(&mutex);
    if (head == client) {
        head = head->next;
    } else {
        struct Client* current = head;
        while (current != NULL && current->next != client) {
            current = current->next;
        }
        if (current != NULL) {
            current->next = client->next;
        }
    }
    close(client->socket);
    free(client);
    pthread_mutex_unlock(&mutex);
}

void* handle_client_custom(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);

    char buffer[BUFFER_SIZE];
    int bytes_received;

    // Prompt for username
    bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    buffer[bytes_received] = '\0'; // Null-terminate the received data
    struct Client* new_client = (struct Client*)malloc(sizeof(struct Client));
    new_client->socket = client_socket;
    strncpy(new_client->username, buffer, sizeof(new_client->username) - 1);
    new_client->username[sizeof(new_client->username) - 1] = '\0';

    pthread_mutex_lock(&mutex);
    new_client->next = head;
    head = new_client;
    pthread_mutex_unlock(&mutex);

    printf("connected_client %s\n", new_client->username);

    while (1) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0 || strcmp(buffer, "exit") == 0) {
            printf("disconnected_client %s \n", new_client->username);
            remove_client_custom(new_client);
            break;
        }
        buffer[bytes_received] = '\0';
        send_message_to_clients_custom(new_client, buffer);
    }

    pthread_exit(NULL);
}

int main() {
    int server_socket, *client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t tid;

    // Create a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    // Bind the socket to an address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8000);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        exit(1);
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) == -1) {
        perror("Listening failed");
        exit(1);
    }

    printf("Server is awaiting incoming connections...\n");

    while (1) {
        // Accept a client connection
        client_socket = (int*)malloc(sizeof(int));
        *client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (*client_socket == -1) {
            perror("Acceptance failed");
            continue;
        }

        // Create a new thread to handle client communication
        pthread_create(&tid, NULL, handle_client_custom, client_socket);
    }

    // Close the server socket
    close(server_socket);
    return 0;
}

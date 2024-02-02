#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

int client_socket;
char username[50];

void* receive_messages_thread(void* arg) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    while (1) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            printf("Server disconnected.\n");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
        if (strcmp(buffer, "exit") == 0) {
            printf("You have disconnected.\n");
            break;
        }
    }
    pthread_exit(NULL);
}

int main() {
    struct sockaddr_in server_addr;
    pthread_t receive_tid;

    // Create a socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    // Connect to the server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        exit(1);
    }

    printf("Connected to server...\n\n");

    // Prompt for username
    printf("Enter your Username: ");
    if (fgets(username, sizeof(username), stdin) != NULL) {
        username[strcspn(username, "\n")] = '\0'; // Remove the trailing newline
    }

    send(client_socket, username, strlen(username), 0);

    printf("\nEnter your messages here (enter 'exit' to terminate)\n");

    // Create a thread to receive messages
    pthread_create(&receive_tid, NULL, receive_messages_thread, NULL);

    // Send messages to the server
    char message[BUFFER_SIZE];
    while (1) {
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        send(client_socket, message, strlen(message), 0);
        if (strcmp(message, "exit") == 0) {
            break;
        }
    }

    printf("\nYou terminated the chat\n");

    // Close the client socket
    close(client_socket);

    return 0;
}

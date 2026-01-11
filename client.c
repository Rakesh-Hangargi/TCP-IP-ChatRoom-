#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct IDS
{
    int client_fd;
    int outgoing_id;
} IDS;

IDS ids;
char user_name[20];

void* listening_func(void* arg)
{
    IDS* id = (IDS*)arg;
    char buffer[1024];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(id->client_fd, buffer, sizeof(buffer)-1, 0);

        if (n <= 0)
        {
            printf("Server disconnected\n");
            break;
        }

        buffer[n] = '\0';
        printf("%s\n", buffer);
    }
    return NULL;
}

void* sending_func(void* arg)
{
    IDS* id = (IDS*)arg;
    char msg[1024];
    char final_msg[1200];

    while(1)
    {
        //printf("Enter your message : ");
        scanf(" %[^\n]", msg);

        // GROUP MODE
        if(id->outgoing_id == -1)
        {
            sprintf(final_msg, "GROUP|%s|%s", user_name, msg);
        }
        // SINGLE USER CHAT
        else
        {
            sprintf(final_msg, "USER|%s|%d|%s", user_name, id->outgoing_id, msg);
        }

        send(id->client_fd, final_msg, strlen(final_msg)+1, 0);
    }
    return NULL;
}

int main()
{
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd < 0)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(6333);    // SERVER PORT
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if(connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        exit(1);
    }

    printf("Connected to server.\n");

    int choice;
    printf("1.Sign-Up\n2.Login\nEnter your choice: ");
    scanf("%d", &choice);

    while(1)
    {
        char password[20];
        printf("Enter username: ");
        scanf("%s", user_name);
        printf("Enter password: ");
        scanf("%s", password);

        char packet[200];

        if(choice == 1)
        {
            sprintf(packet, "SIGNUP|%s|%s", user_name, password);
        }
        else
        {
            sprintf(packet, "LOGIN|%s|%s", user_name, password);
        }

        send(client_fd, packet, strlen(packet)+1, 0);

        int ack;
        recv(client_fd, &ack, sizeof(ack), 0);

        if(choice == 1)
        {
            if(ack == 0)
            {
                printf("Username exists. Try again.\n");
            }
            else
            {
                printf("Signup successful. Login now.\n");
                choice = 2;
            }
        }
        else
        {
            if(ack == 1)
            {
                printf("Login successful.\n");
                break;
            }
            else if(ack == 0)
            {
                printf("Wrong password.\n");
            }
            else if(ack == 2)
            {
                printf("Username not found.\n");
            }
        }
    }

    printf("1) Single Chat\n2) Group Chat\nEnter your choice: ");
    scanf("%d", &choice);

    if(choice == 1)
    {
        int target;
        printf("Enter fd of user you want to talk to: ");
        scanf("%d", &target);
        ids.outgoing_id = target;
    }
    else
    {
        ids.outgoing_id = -1;  // group chat mode
    }

    ids.client_fd = client_fd;

    pthread_t t1, t2;
    pthread_create(&t1, NULL, listening_func, &ids);
    pthread_create(&t2, NULL, sending_func, &ids);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}

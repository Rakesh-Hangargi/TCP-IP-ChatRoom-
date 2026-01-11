#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX 1024

typedef struct login
{
    int fd;
    char name[50];
    char pswd[50];
} cred;

cred logins[MAX];
int clients[MAX];
 int total = 0;

void *listener_func(void *arg)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(6333);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    listen(sock, 10);

    fd_set readfds;
    int max_fd;
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        max_fd = sock;

        for (int i = 0; i < MAX; i++)
        {
            if (clients[i] != -1)
            {
                FD_SET(clients[i], &readfds);
                if (clients[i] > max_fd)
                    max_fd = clients[i];
            }
        }

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        // New connection
        if (FD_ISSET(sock, &readfds))
        {
            struct sockaddr_in claddr;
            socklen_t len = sizeof(claddr);

            int new_client = accept(sock, (struct sockaddr *)&claddr, &len);

            if (new_client > 0)
            {
                printf("New client connected: fd = %d\n", new_client);

                for (int i = 0; i < MAX; i++)
                {
                    if (clients[i] == -1)
                    {
                        clients[i] = new_client;
                        break;
                    }
                }
            }
        }

        // Existing client sent something
        for (int i = 0; i < MAX; i++)
        {
            int fd = clients[i];
            if (fd != -1 && FD_ISSET(fd, &readfds))
            {
                char buffer[1024];
                int n = recv(fd, buffer, sizeof(buffer) - 1, 0);

                if (n <= 0)
                {
                    printf("Client %d disconnected\n", fd);
                    char str[100];
                    sprintf(str,"User %d is OFFLINE\n",fd);
                    for(int i=0;i<MAX;i++)
                    {
                        if(clients[i]!=-1 && clients[i]!=fd)
                        {
                            send(clients[i],str,strlen(str)+1,0);
                        }
                    }
                    close(fd);
                    clients[i] = -1;
                }
                else
                {
                    buffer[n] = '\0';
                    printf("Message from client %d: %s\n", fd, buffer);

                    // NEXT STEP: PROCESS + FORWARD TO TARGET USER
                    char type[7];
                    sscanf(buffer, "%[^|]", type);
                    if ((!strcmp(type, "LOGIN")) || (!strcmp(type, "SIGNUP")))
                    {
                        char username[20];
                        char password[20];
                        int ack = 0;
                        sscanf(buffer, "%[^|]|%[^|]|%[^\n]", type, username, password);
                        if (!strcmp("LOGIN", type))
                        {
                            int logged = 0;
                            for (int i = 0; i < total; i++)
                            {
                                if (strcmp(username, logins[i].name) == 0)
                                {
                                    if (strcmp(password, logins[i].pswd) == 0)
                                    {
                                        ack = 1;
                                        logged = 1;
                                        send(fd, &ack, sizeof(int), 0);
                                    }
                                    else
                                    {
                                        ack = 0;
                                        logged = 1; // FIXED
                                        send(fd, &ack, sizeof(int), 0);
                                    }
                                    break;
                                }
                            }

                            if (!logged)
                            {
                                ack = 2;
                                send(fd, &ack, sizeof(int), 0);
                            }
                        }
                        else
                        {
                            int flag = 1;
                            for (int i = 0; i < total; i++)
                            {
                                if (!strcmp(logins[i].name, username))
                                {
                                    int ack = 3;
                                    send(fd, &ack, sizeof(int), 0);
                                    flag = 0;
                                    break;
                                }
                            }

                            if (flag)
                            {
                                logins[total].fd = fd;
                                strcpy(logins[total].name, username);
                                strcpy(logins[total].pswd, password);
                                total++;

                                int ack = 1;
                                send(fd, &ack, sizeof(int), 0);
                            }
                        }
                    }
                    else if (!strcmp(type, "GROUP"))
                    {
                        char username[20];
                        char message[100];
                        sscanf(buffer, "%[^|]|%[^|]|%[^\n]", type, username, message);

                        for (int i = 0; i < MAX; i++)
                        {
                            if (clients[i] == -1)
                                continue;

                            char f_message[100];

                            if (clients[i] == fd)
                            {
                                strcpy(f_message, "You : ");
                                strcat(f_message, message);
                            }
                            else
                            {
                                strcpy(f_message, username);
                                strcat(f_message, " : ");
                                strcat(f_message, message);
                            }

                            send(clients[i], f_message, strlen(f_message) + 1, 0);
                        }
                    }
                    else if (!strcmp(type, "USER"))
                    {
                        char username[20];
                        char message[100];
                        int target;
                        sscanf(buffer, "%[^|]|%[^|]|%d|%[^\n]", type, username, &target, message);
                        char f_message[100];
                        strcpy(f_message, username);
                        strcat(f_message, " : ");
                        strcat(f_message, message);
                        send(target, f_message, strlen(f_message) + 1, 0);
                    }
                }
            }
        }
    }

    return NULL;
}

int main()
{
    printf("\t\tTCP/IP CHATROOM\n");

    for (int i = 0; i < MAX; i++)
        clients[i] = -1;

    pthread_t listener;
    pthread_create(&listener, NULL, listener_func, NULL);

    pthread_join(listener, NULL);
}

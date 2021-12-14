#include "functions.h"

int sd;
char name[16];

void send_data();
void recv_data();

int main()
{
    int uid=-1;
    char initial[52], char_uid[4], *command, *username, *password;
    pthread_t send_thread, recv_thread;

    printf("To create an account: /register <username>/<password>.\n");
    printf("To log in: /login <username>/<password>.\n\n");

    do {
        bzero(initial, 52); fgets(initial, 52, stdin); initial[strlen(initial)-1]='\0';
        command=strtok(initial, " ");
        username=strtok(NULL, "/");
        password=strtok(NULL, "");

        if(strcmp(command, "/register")!=0 && strcmp(command, "/login")!=0 && strcmp(command, "/quit")!=0)
            printf("Unknown command.\n");
        else if(strcmp(command, "/quit")==0)
            exit(0);
        else if(username==NULL || password==NULL)
            printf("Incorrect syntax. The format is: %s <username>/<password>.\n", command);
        else if(strcmp(command, "/register")==0) //making an account
        {
            int result=newuser(username, password);
            if(result==-3)
                printf("Username cannot be longer than 15 characters.\n");
            else if(result==-2)
                printf("Password cannot be longer than 25 characters.\n");
            else if(result==-1)
                printf("Username contains characters that are not allowed (spaces or '\\').\n");
            else if(result==0)
                printf("User already exists.\n");
            else
                printf("Account created successfully. You can now log in.\n");
        }
        else if(strcmp(command, "/login")==0) //logging in
        {
            uid=login(username, password);
            if(uid==-2)
                printf("User does not exist.\n");
            else if(uid==-1)
                printf("Username or password is incorrect.\n");
        }
    } while(uid<0);

    sd=socket(AF_INET, SOCK_STREAM, 0);
    if(sd==-1)
    {
        perror("Error creating socket.\n");
        return errno;
    }

    struct sockaddr_in server;
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=inet_addr("127.0.0.1");
    server.sin_port=htons(2000);

    if(connect(sd, (struct sockaddr *)&server, sizeof(server))==-1)
    {
        perror("Error connecting to server.\n");
        return errno;
    }

    bzero(name, 16); bzero(char_uid, 4);
    strcat(name, username); sprintf(char_uid, "%d", uid);
    int sendname=send(sd, name, 16, 0);
    int senduid=send(sd, char_uid, 4, 0);
    if(sendname==-1 || senduid==-1)
    {
        perror("Error writing to server.\n");
        return errno;
    }

    printf("Type '/help' for information on how to use.\n\n");

    if(pthread_create(&send_thread, NULL, (void *)send_data, NULL)!=0)
    {
        printf("Error creating thread.\n");
        return errno;
    }

    if(pthread_create(&recv_thread, NULL, (void *)recv_data, NULL)!=0)
    {
        printf("Error creating thread.\n");
        return errno;
    }

    while(1)
    {
        //program runs until user inputs "/quit"
    }

    close(sd);
}

void send_data()
{
    char message[250], buffer[270];

    while(1)
    {
        fgets(message, 250, stdin); message[strlen(message)-1]='\0';
        if(strcmp(message, "/quit")==0)
        {
            char path[40];
            bzero(path, 40); strcat(path, "users/"); strcat(path, name); strcat(path, "_unread.txt");
            FILE *unreadfile=fopen(path, "w"); //empty file when closing app
            fclose(unreadfile);
            exit(0);
        }
        else if(strncmp(message, "/register", 9)==0 || strncmp(message, "/login", 6)==0)
            printf("You are already logged in. If you want to create an account or log into a different one, you must first log out.\n");
        else
        {
            if(strncmp(message, "/help", 5)==0 || strncmp(message, "/view", 5)==0 || strncmp(message, "/to", 3)==0 ||
            strncmp(message, "/reply", 6)==0 || strncmp(message, "/history", 8)==0 ||
            strncmp(message, "/chat-history", 13)==0 || strncmp(message, "/online-users", 13)==0) //commands
                sprintf(buffer, "%s", message);
            else //message everyone
                sprintf(buffer, "%s %s: %s\n", current_time(), name, message);

            if(send(sd, buffer, strlen(buffer), 0)==-1)
            {
                perror("Error writing to server.\n");
                exit(0);
            }
        }
        bzero(message, 250); bzero(buffer, 270);
    }
}

void recv_data()
{
    char buffer[1000], path[40];

    bzero(path, 40); strcat(path, "users/"); strcat(path, name); strcat(path, "_unread.txt");
    if(file_size(path)>0)
        printf("You have new messages. Type '/view' to read them.\n");

    while(1)
    {
        int length=recv(sd, buffer, 1000, 0);
        if(length==-1)
        {
            perror("Error reading from server.\n");
            exit(0);
        }
        else if(length==0)
            break;
        else
        {
            printf("%s", buffer); fflush(stdout);
        }
        bzero(buffer, 1000);
    }
}

#include "functions.h"

typedef struct
{
    char username[16];
  	int fd;
  	int uid;
} Client;

Client *clients[10];
pthread_mutex_t clients_mutex=PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg);
void add_client(Client *cl);
void remove_client(Client *cl);
void send_message(int uid, char message[240]);
void online_users(char response[1000]);

int main()
{
    pthread_t tid;
    int optval=1;

    int sd=socket(AF_INET, SOCK_STREAM, 0);
    if(sd==-1)
    {
        perror("Error creating socket.\n");
        return errno;
    }

    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in server;
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=inet_addr("127.0.0.1");
    server.sin_port=htons(2000);

    if(bind(sd, (struct sockaddr *)&server, sizeof(server))==-1)
    {
        perror("Error on bind().\n");
        return errno;
    }

    if(listen(sd, 3)==-1)
    {
        perror("Error on listen().\n");
        return errno;
    }

  	while(1)
    {
    		int client=accept(sd, NULL, NULL);

    		Client *cli=(Client *)malloc(sizeof(Client));
    		cli->fd=client;

    		add_client(cli);
    		pthread_create(&tid, NULL, &handle_client, (void*)cli);

        sleep(1); //reduce CPU usage
  	}
}

void *handle_client(void *arg) //communicate with client
{
  	char buffer[256], response[1000], name[16], char_uid[4]="-1";
  	int offline=0;
  	Client *cli=(Client *)arg;

    recv(cli->fd, name, 16, 0); strcpy(cli->username, name); //get username
    recv(cli->fd, char_uid, 4, 0); cli->uid=atoi(char_uid); //get user id

		sprintf(buffer, "%s is now online.\n", cli->username);
    printf("[UID %d] %s", cli->uid, buffer); fflush(stdout);
		send_message(cli->uid, buffer);

  	while(offline==0)
    {
        bzero(buffer, 256); bzero(response, 1000);
        int length=recv(cli->fd, buffer, 256, 0);

    		if(length==-1)
        {
            perror("Error reading from client.\n");
            offline=1;
        }
        else if(length==0)
        {
      			sprintf(buffer, "%s is now offline.\n", cli->username);
            printf("[UID %d] %s", cli->uid, buffer); fflush(stdout);
      			send_message(cli->uid, buffer);
      			offline=1;
    		}
        else
        {
            char aux[256]; strcpy(aux, buffer);
            char *command=strtok(aux, " ");

            if(strcmp(command, "/help")==0)
            {
                char *token=strtok(NULL, "");

                if(token!=NULL)
                    sprintf(response, "Incorrect syntax. The format is: /help.\n");
                else
                {
                    sprintf(response, "- /help -- view details about how to use the app\n");
                    strcat(response, "- /register <username>/<password> -- create new account\n");
                    strcat(response, "- /login <username>/<password> -- log in to an account\n");
                    strcat(response, "- <text> -- message all online users (maximum length of message is 236 characters)\n");
                    strcat(response, "- /to <username>: <text> -- send private message to <username>\n");
                    strcat(response, "- /reply <message id>: <text> -- reply to the message with the id <message id>\n");
                    strcat(response, "- /view -- view messages received while offline\n");
                    strcat(response, "- /history -- view your own message history\n");
                    strcat(response, "- /history <username> -- view your message history with <username>\n");
                    strcat(response, "- /chat-history -- view chat history\n");
                    strcat(response, "- /online-users -- view list of online users\n");
                    strcat(response, "- /quit -- exit the app\n");
                }
            }
            else if(strcmp(command, "/to")==0)
            {
                char *username=strtok(NULL, ":");
                char *text=strtok(NULL, "");

                if(username==NULL || text==NULL)
                    sprintf(response, "Incorrect syntax. The format is: /to <username>: <text>.\n");
                else
                {
                    int i, is_online=0;
                    for(i=0;i<10;i++)
                        if(clients[i]!=NULL && clients[i]->uid==get_uid(username))
                            is_online=1;

                    if(is_online==0)
                    {
                        sprintf(response, "%s:%s\n", cli->username, text);
                        int result=send_pm(username, response);

                        if(result==-1)
                        {
                            bzero(response, 1000);
                            sprintf(response, "User %s doesn't exist.\n", username);
                        }
                        else
                        {
                            char path_sender[60], path_both[60];

                            sprintf(path_sender, "users/%s_history.txt", cli->username);
                            if(strcmp(cli->username, username)>0)
                                sprintf(path_both, "users/%s_%s_history.txt", username, cli->username);
                            else sprintf(path_both, "users/%s_%s_history.txt", cli->username, username);

                            add_history(path_sender, response);
                            add_history(path_both, response);

                            bzero(response, 1000);
                            sprintf(response, "Message sent to %s.\n", username);
                            printf("%s sent a PM to %s.\n", cli->username, username); fflush(stdout);
                        }
                    }
                    else sprintf(response, "You can't send private messages to users who are online. Try talking in the chat.\n");
                }
            }
            else if(strcmp(command, "/view")==0)
            {
                char *token=strtok(NULL, "");

                if(token!=NULL)
                    sprintf(response, "Incorrect syntax. The format is: /view.\n");
                else view_unread(cli->username, response);
            }
            else if(strcmp(command, "/reply")==0)
            {
                char *mid=strtok(NULL, ":");
                char *text=strtok(NULL, "");

                if(mid==NULL || text==NULL)
                    sprintf(response, "Incorrect syntax. The format is: /reply <message id>: <text>.\n");
                else
                {
                    int reply_sent=0;
                    char path_sender[60], path_both[60], recipient[16];

                    int result=reply_msg(cli->username, atoi(mid), text, response);
                    if(result>=0)
                    {
                        int i;
                        for(i=0;i<10;i++)
                            if(clients[i]!=NULL && clients[i]->uid==result)
                            {
                                if(send(clients[i]->fd, response, strlen(response), 0)==-1)
                                {
                                    perror("Error sending message.\n");
                                    offline=1;
                                }
                                reply_sent=1;
                                bzero(recipient, 16); strcat(recipient, clients[i]->username);
                                break;
                            }

                        if(reply_sent==0) //if user is not online
                        {
                            send_pm(get_username(result), response);
                            bzero(recipient, 16); strcat(recipient, get_username(result));
                        }

                        sprintf(path_sender, "users/%s_history.txt", cli->username);
                        if(strcmp(cli->username, recipient)>0)
                            sprintf(path_both, "users/%s_%s_history.txt", recipient, cli->username);
                        else sprintf(path_both, "users/%s_%s_history.txt", cli->username, recipient);

                        add_history(path_sender, response);
                        add_history(path_both, response);

                        bzero(response, 1000); sprintf(response, "Reply sent successfully.\n");
                        printf("%s replied to a message from %s.\n", cli->username, recipient);
                        fflush(stdout);
                    }
                }
            }
            else if(strcmp(command, "/history")==0)
            {
                char *username=strtok(NULL, "");

                if(username==NULL)
                    view_history(cli->username, NULL, response);
                else view_history(cli->username, username, response);
            }
            else if(strcmp(command, "/chat-history")==0)
            {
                char *token=strtok(NULL, "");

                if(token!=NULL)
                    sprintf(response, "Incorrect syntax. The format is: /chat-history.\n");
                else view_history("\\", NULL, response);
            }
            else if(strcmp(command, "/online-users")==0)
            {
                char *token=strtok(NULL, "");

                if(token!=NULL)
                    sprintf(response, "Incorrect syntax. The format is: /online-users.\n");
                else online_users(response);
            }
            else //chat messages
            {
                char path[60];
                sprintf(path, "users/%s_history.txt", cli->username);

                send_message(cli->uid, buffer);
                add_history("users/history_chat.txt", buffer);
                add_history(path, buffer);
                printf("%s wrote in the chat.\n", cli->username); fflush(stdout);
            }

            if(send(cli->fd, response, strlen(response), 0)==-1)
            {
                perror("Error sending message.\n");
                offline=1;
            }
    		}
  	}

  	close(cli->fd); //done treating client
    remove_client(cli);
    free(cli);
    pthread_detach(pthread_self());

    return NULL;
}

void add_client(Client *cl)
{
  	pthread_mutex_lock(&clients_mutex);
  	for(int i=0;i<10;i++)
    		if(clients[i]==NULL)
        {
      			clients[i]=cl;
      			break;
    		}
  	pthread_mutex_unlock(&clients_mutex);
}

void remove_client(Client *cl)
{
  	pthread_mutex_lock(&clients_mutex);
  	for(int i=0;i<10;i++)
    		if(clients[i]==cl)
        {
    				clients[i]=NULL;
    				break;
  			}
  	pthread_mutex_unlock(&clients_mutex);
}

void send_message(int uid, char message[240])
{
  	pthread_mutex_lock(&clients_mutex);
  	for(int i=0;i<10;i++)
        if(clients[i]!=NULL && clients[i]->uid!=uid)
    				if(send(clients[i]->fd, message, strlen(message), 0)==-1)
            {
      					perror("Error writing to client.\n");
      					break;
    				}
  	pthread_mutex_unlock(&clients_mutex);
}

void online_users(char response[1000])
{
    pthread_mutex_lock(&clients_mutex);
  	for(int i=0;i<10;i++)
        if(clients[i]!=NULL)
    		{
            strcat(response, clients[i]->username);
            strcat(response, "\n");
        }
  	pthread_mutex_unlock(&clients_mutex);
}

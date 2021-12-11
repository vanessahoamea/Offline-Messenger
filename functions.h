#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

int get_uid(char user[16]);
char* get_username(int id);
int file_size(char path[30]);
int newuser(char user[16], char pass[26]);
int login(char user[16], char pass[26]);
int send_pm(char user[16], char text[236]);
void view_unread(char user[16], char response[1000]);
int reply_msg(char user[16], int mid, char text[240], char response[1000]);
void add_history(char path[30], char text[260]);
int view_history(char user1[16], char user2[16], char response[1000]);

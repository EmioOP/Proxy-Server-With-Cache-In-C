#include "proxy_parse.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<pthread.h>
#include<semaphore.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<unistd.h>
#include<strings.h>

#define MAX_CLIENTS 10
#define MAX_BYTES 4096
typedef struct cache_element cache_element;

struct cache_element{
    char* data;
    int len;
    char* url;
    time_t lru_time_track;
    cache_element* next;
};

cache_element* find(char* url);
int add_cache_element(char* data,int size,char* url);
void remove_cache_element();

int port_number = 8080;

int proxy_socket_id;

pthread_t tid[MAX_CLIENTS];
sem_t semaphore;
pthread_mutex_t lock;

cache_element* head;
int cache_size;


void *thread_fn(void *socketNew){
    sem_wait(&semaphore);
    int p;
    sem_getvalue(&semaphore,p);
    printf("semaphore value is: %d\n",p);
    int *t = (int *)socketNew;
    int socket = *t;
    int bytes_send_client,len;
    char *buffer = (char*)calloc(MAX_BYTES,sizeof(char));
    bzero(buffer,MAX_BYTES);
    bytes_send_client = recv(socket,buffer,MAX_BYTES,0);

    while (bytes_send_client > 0){
        len=strlen(buffer);
        if(strstr(buffer,"\r\n\r\n") == NULL){ // "\r\n\r\n" -> http request has the charecter arrangement in the end - header terminat. means we have to accept the request till the end.
            bytes_send_client = recv(socket,buffer+len,MAX_BYTES-len,0);

        }else{
            break;
        }
    }

    char *tempReq = (char *)malloc(strlen(buffer) * sizeof(char));
    for(int i=0;i<strlen(buffer);i++){
        tempReq[i] = buffer[i];
    }

    struct cache_element* temp = find(tempReq);
    if(temp != NULL){
        int size = temp->len/sizeof(char);
        int pos = 0;
        char response[MAX_BYTES];
        while (pos < size){
            // continue from here..............
        }
        
    }




    


}

int main(int argc,char* argv[]){
    int client_socketId,client_len;
    struct sockaddr_in server_addr,client_addr;

    sem_init(&semaphore,0,MAX_CLIENTS);
    pthread_mutex_init(&lock,NULL);


    if(argc == 2){
        port_number = atoi(argv[1]);
    }else{
        printf("Too few arguments");
        exit(1);
    }


    printf("Starting Proxy server at port %d",port_number);

    proxy_socket_id = socket(AF_INET,SOCK_STREAM,0);

    if(proxy_socket_id<0){
        perror("Failed to create a socket\n");
        exit(1);
    }

    int reuse = 1;
    if(setsockopt(proxy_socket_id,SOL_SOCKET,SO_REUSEADDR,(const char*)&reuse,sizeof(reuse)) < 0){
        perror("SetSockOpt Error\n");
    }

    bzero((char *)&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(proxy_socket_id,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("Socket binding failed\n");
        exit(1);
    }

    printf("Binding on port:%d",port_number);

    int listen_status = listen(proxy_socket_id,MAX_CLIENTS);
    if(listen_status<0){
        perror("Error in listening\n");
        exit(1);
    }

    int i =0;
    int Connected_SocketId[MAX_CLIENTS];
    

    while(1){
        bzero((char *)&client_addr,sizeof(client_addr));
        client_len =sizeof(client_addr);
        client_socketId = accept(proxy_socket_id,(struct sockaddr*)&client_addr,&client_len);
        if(client_socketId < 0){
            printf("Not able to conenct\n");
            exit(1);
        }else{
            Connected_SocketId[i] = client_socketId;
        }

        struct sockaddr_in * client_pt = (struct sockaddr_in*)&client_addr;
        struct in_addr ip_addr = client_pt->sin_addr;
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,&ip_addr,str,INET_ADDRSTRLEN);
        printf("Client is connected with port :%d and ip address:%s\n",ntohs(client_addr.sin_port),str);

        pthread_create(&tid[i],NULL,thread_fn,(void *)&Connected_SocketId[i]);
        i++;
    }
    close(proxy_socket_id);
    return 0;
}
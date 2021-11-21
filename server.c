#include "tands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define MAX_MSG_LEN 255
#define MAX_CLIENT 11
#define TIMEOUT_SEC 30

struct timespec last_op_time, start_time;
int t_cnt,cli_cnt;
char cli_name[MAX_CLIENT][255];
int cli_tcnt[MAX_CLIENT];

void err(char *disp){
    printf("Error: %s\n", disp);
    exit(-1);
}

void print_log(int tn, int para, char *host) {
    char s_para[5];
    if (para<0) {sprintf(s_para,"Done");}
    else
    {
        sprintf(s_para,"T%3d",para);
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);
    printf("%ld.%02ld: #%3d (%s) from %s\n",ts.tv_sec,ts.tv_nsec/10000000,tn+1,s_para,host);
}

void error(char *msg){
    fprintf(stdout,"Error %s.\n",msg);
    exit(1);
}

void parse_msg(char *msg) {
    char cmd;
    int para,rc;
    rc=sscanf(msg,"%c%d",&cmd,&para);
    if (rc!=2) error("Invalid command format received.");
    if (cmd!='T') {
        char es[50];
        sprintf(es,"Invalid command \"%c\" received.",cmd);
        error(es);
    } else {
        print_log(t_cnt,para,cli_name[cli_cnt]);
        Trans(para);
        print_log(t_cnt,-1,cli_name[cli_cnt]);
    }
}

void tick(){
    clock_gettime(CLOCK_MONOTONIC, &last_op_time);
}

int timeout(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ts.tv_sec-last_op_time.tv_sec>TIMEOUT_SEC) {
        return 1;
    } else {
        return 0;
    }
}

int main(int argc, char *argv[]){
    if (argc<2){
        err("Invalid argument");
    }

    int port=atoi(argv[1]);
    printf("Using port %d\n", port);

    int socket_desc , client_sock , c , read_size;
	struct sockaddr_in server , client;
	char message[MAX_MSG_LEN];
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM|SOCK_NONBLOCK , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");

    //Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( port );

    //Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");
	
	//Listen
	listen(socket_desc , MAX_CLIENT);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");

    tick();
    while (!timeout()){
        c = sizeof(struct sockaddr_in);
	
        //accept connection from an incoming client
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0)
        {
            if (errno==EAGAIN || errno==EWOULDBLOCK){
                continue;
            }
            perror("accept failed");
            return 1;
        }

        getnameinfo((struct sockaddr *) &client, c, cli_name[cli_cnt], sizeof(cli_name[cli_cnt]), NULL, 0, NI_NAMEREQD);
        fprintf(stdout,"[DEBUG] Connection accepted from %s\n", cli_name[cli_cnt]);

        while (!timeout()){
            //Receive a message from client
            while( (read_size = recv(client_sock , message , MAX_MSG_LEN , 0)) > 0 )
            {
                if (t_cnt==0) clock_gettime(CLOCK_REALTIME, &start_time);
                parse_msg(message);
                //Send the message back to client
                cli_tcnt[cli_cnt]++;
                t_cnt++;
                sprintf(message,"D%d",t_cnt);
                write(client_sock , message , strlen(message));
                tick();
            }
            
            if(read_size == 0)
            {
                puts("Client disconnected");
                fflush(stdout);
                break;
            }
            else if(read_size == -1)
            {   
                if (errno==EAGAIN || errno==EWOULDBLOCK){
                    continue;
                }
                perror("recv failed");
            }
        }
        cli_cnt++;
    }
    printf("SUMMARY\n");
    for (int i=0;i<cli_cnt;i++) printf("%d transactions from %s\n",cli_tcnt[i],cli_name[i]);
    double elap=last_op_time.tv_sec-start_time.tv_sec+(last_op_time.tv_nsec-start_time.tv_nsec)*1e-9;
    printf("%.1f transactions/sec (%d/%.2f)\n",t_cnt/elap, t_cnt, elap);

    return 0;
}

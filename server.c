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

#define MAX_MSG_LEN 300
#define MAX_CLIENT 11
#define TIMEOUT_SEC 30

struct timespec last_op_time, start_time;
int t_cnt,cli_cnt;
char cli_name[MAX_CLIENT][MAX_MSG_LEN];
int cli_tcnt[MAX_CLIENT];

void print_log(int tn, int para, char *host) {
    char s_para[12];
    if (para<0) {sprintf(s_para,"Done");}
    else
    {
        sprintf(s_para,"T%3d",para);
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);
    printf("%ld.%02ld: #%3d (%s) from %s\n",ts.tv_sec,ts.tv_nsec/10000000,tn+1,s_para,host);
}

void error(char *msg,int ev){
    // print out error message and exit
    if (ev>0) fprintf(stdout,"Error %s%s.\n",msg ,strerror(ev));
        else fprintf(stdout,"Error %s\n", msg);
    exit(1);
}

void parse_msg(char *msg) {
    char cmd;
    int para,rc;
    rc=sscanf(msg,"%c%d",&cmd,&para);
    if (rc!=2) error("Invalid command format received.", EINVAL);  // invalid command from client
    if (cmd!='T') {
        char es[50];
        sprintf(es,"Invalid command \"%c\" received.",cmd);
        error(es, EINVAL);
    } else {
        print_log(t_cnt,para,cli_name[cli_cnt]);
        Trans(para);  // execute the actual transaction
        print_log(t_cnt,-1,cli_name[cli_cnt]);
    }
}

void tick(){
    clock_gettime(CLOCK_MONOTONIC, &last_op_time);
}

int timeout(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    // check if timer is up
    // we ignore minor differences in tv_nsec, the tv_sec part is precise enough for this purpose
    if (ts.tv_sec-last_op_time.tv_sec>TIMEOUT_SEC) {
        return 1;
    } else {
        return 0;
    }
}

int main(int argc, char *argv[]){
    // invalid argument
    if (argc<2){
        error("Invalid command line argument",EINVAL);
    }

    // get port number from command-line argument
    int port=atoi(argv[1]);
    if (port<5000||port>64000) error("Invalid port range",EINVAL);
    printf("Using port %d\n", port);

    int socket_desc , client_sock , c , read_size;
	struct sockaddr_in server , client;
	char message[MAX_MSG_LEN];
	
	// create socket
	socket_desc = socket(AF_INET , SOCK_STREAM|SOCK_NONBLOCK , 0);
	if (socket_desc == -1) error("Could not create socket", errno);

    // initialize the sockaddr_in structure for server
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( port );

    // bind and check error
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) error("Bind error:", errno);	
	
	// listen
	listen(socket_desc , MAX_CLIENT);
	
	// accept incoming connection
    tick();
    while (!timeout()){
        c = sizeof(struct sockaddr_in);
	
        // accept an incoming connection (non-blocking)
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0)
        {
            // non-blocking accept check, keep looping if no connection pending
            if (errno==EAGAIN || errno==EWOULDBLOCK){
                continue;
            }
            perror("accept failed");
            return 1;
        }

        // get client host name
        // int gn_result=getnameinfo((struct sockaddr *) &client, c, cli_name[cli_cnt], sizeof(cli_name[cli_cnt]), NULL, 0, NI_NAMEREQD);
        // if (gn_result!=0) error((char *) gai_strerror(gn_result),0);
        

        // loop until timeout
        while (!timeout()){
            //Receive a message from client
            while( (read_size = recv(client_sock , message , MAX_MSG_LEN , 0)) > 0 )
            {
                if (t_cnt==0) {
                    clock_gettime(CLOCK_MONOTONIC, &start_time);  // record first transaction time
                    sscanf(message,"%s\n",cli_name[cli_cnt]);
                    sprintf(message,"%s\n",cli_name[cli_cnt]);
                } else {
                    // parse received command message
                    parse_msg(message);
                    cli_tcnt[cli_cnt]++;
                    t_cnt++;
                    sprintf(message,"D%d\n",t_cnt);
                }
                if (write(client_sock , message , strlen(message))<0)  //Send the respose message back to client
                    error("Write error", errno);
                tick();  // reset timer
            }
            
            if(read_size == 0)
            {
                // client disconnected
                fflush(stdout);
                break;
            }
            else if(read_size == -1)
            {   
                // non-blocking read check, keep looping if no pending message
                if (errno==EAGAIN || errno==EWOULDBLOCK){
                    continue;
                }
                // actual error occurred
                error("Recv failed",errno);
            }
        }
        cli_cnt++;
    }
    printf("SUMMARY\n");
    for (int i=0;i<cli_cnt;i++) printf("%d transactions from %s\n",cli_tcnt[i],cli_name[i]);
    // calculate elapsed time
    double elap=0.0;
    if (t_cnt>0){
        elap=last_op_time.tv_sec-start_time.tv_sec+(last_op_time.tv_nsec-start_time.tv_nsec)*1e-9;
        printf("%.1f transactions/sec (%d/%.2f)\n",t_cnt/elap, t_cnt, elap);
    } else {
        printf("0 transactions/sec (0/0)\n");
    }        

    return 0;
}

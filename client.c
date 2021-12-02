#include "tands.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_MSG_LEN 300

FILE *fout;
int sock, snd_cnt;

// function to print a log entry
void print_log(int type, char cmd, int para) { // type = -1: sleep, 0: send, 1: recv;
    if (type==-1){
        fprintf(fout,"Sleep %d units\n", para);
    } else {
        char s_type[5];
        if (type==0) sprintf(s_type,"Send");
            else sprintf(s_type,"Recv");
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        fprintf(fout,"%ld.%02ld: %s (%c %3d)\n", ts.tv_sec, ts.tv_nsec/10000000, s_type, cmd, para);
    }    
}

// print out error message and exit
void error(char *msg){
    fprintf(stdout,"Error: %s\n", msg);
    close(sock);
    fclose(fout);
    exit(-1);
}

int main(int argc, char *argv[]){
    if (argc<3){
        error("Invalid argument");
    }
    // get command-line arguments
    char *str_srv_addr=argv[2];
    int port=atoi(argv[1]);

    // get client hostname
    char local_name[255], log_fn[270];
    gethostname(local_name,255);

    // generate log file name
    pid_t pid;
    pid=getpid();
    sprintf(log_fn,"%s.%d",local_name,(int) pid);

    // open log file
    fout=fopen(log_fn,"w");

    // print out info
    fprintf(fout,"Using port %d\n",port);
    fprintf(fout,"Using server address %s\n", str_srv_addr);
    fprintf(fout,"Host %s\n",log_fn);
    
    // create socket
    sock=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in srv_addr={.sin_addr.s_addr=inet_addr(str_srv_addr),.sin_family=AF_INET,.sin_port=htons(port)};

    // connect socket
    if (connect(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0)
	{
		perror("Connect failed. Error: ");
		return 1;
	}

    // send hostname and pid to server
    char fmsg[MAX_MSG_LEN];
    sprintf(fmsg,"%s\n",log_fn);
    if (send(sock, fmsg, strlen(fmsg), 0) < 0) error("Send failed");

    // read acknowledge message from server
    char ack[MAX_MSG_LEN];
    if (recv(sock, ack, MAX_MSG_LEN, 0) < 0) error("recv failed");
    ack[strcspn(ack, "\n")] = 0;

    // validate message
    if (strcmp(log_fn,ack)!=0) error("Validation error");

    // start reading and sending commands
    int rc,para;
    char s_in[255],cmd;
    while (fgets(s_in,255,stdin)!=NULL) {
        // read command from stdin
        rc=sscanf(s_in,"%c%d",&cmd,&para);
        // check command format
        if (rc!=2) error("Invalid command");
        // process commands
        switch (cmd)
        {
        case 'T':
            // print to log
            print_log(0,'T',para);
            // send to server
            char message[MAX_MSG_LEN], reply[MAX_MSG_LEN];
            memset(message, 0, sizeof(message));
            sprintf(message,"T%d\n",para);
            if (send(sock, message, MAX_MSG_LEN, 0) < 0) error("Send failed");
            snd_cnt++;
            // wait for and read reply (done message) from server
            if (recv(sock, reply, MAX_MSG_LEN, 0) < 0) error("recv failed");
            // validate server reply
            char rcmd;
            int rpara;
            int pc=sscanf(reply,"%c%d\n",&rcmd,&rpara);
            if (pc!=2||rcmd!='D') error("Invalid reply");
            // print log
            print_log(1,'D',rpara);
            break;
        case 'S':
            // sleep
            print_log(-1, 0, para);
            Sleep(para);
            break;
        default:
            error("Invalid command.");
            break;
        }
    }
    fprintf(fout,"Sent %d transactions\n",snd_cnt);
    close(sock);
    fclose(fout);
    return 0;
}
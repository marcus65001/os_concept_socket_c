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
    char *port=argv[1];

    // get server address from hostname
    struct addrinfo *res, *rp;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    int s_ai = getaddrinfo(str_srv_addr,port,&hints,&res);
    if (s_ai != 0) {
        error((char *)gai_strerror(s_ai));
    }

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
    fprintf(fout,"Using port %s\n",port);
    fprintf(fout,"Using server address %s\n", str_srv_addr);
    fprintf(fout,"Host %s\n",local_name);
    
    // create socket
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype,
                    rp->ai_protocol);
        if (sock == -1)
            continue;

        if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(sock);
    }
    freeaddrinfo(res);

    // check if any address succeeded
    if (rp == NULL) {
        error("Could not connect to server.");
        exit(EXIT_FAILURE);
    }
    char fmsg[MAX_MSG_LEN];
    sprintf(fmsg,"%s\n",log_fn);
    if( send(sock, fmsg, strlen(fmsg), 0) < 0) error("Send failed");
    char ack[MAX_MSG_LEN];
    if( recv(sock, ack, MAX_MSG_LEN, 0) < 0) error("recv failed");
    ack[strcspn(ack, "\n")] = 0;
    if (strcmp(log_fn,ack)!=0) error("Validation error");
    int rc,para;
    char s_in[255],cmd;
    while (fgets(s_in,255,stdin)!=NULL) {
        rc=sscanf(s_in,"%c%d",&cmd,&para);
        if (rc!=2) error("Invalid command");
        switch (cmd)
        {
        case 'T':
            print_log(0,'T',para);
            char message[MAX_MSG_LEN], reply[MAX_MSG_LEN];
            sprintf(message,"T%d\n",para);
            if( send(sock, message, strlen(message), 0) < 0) error("Send failed");
            snd_cnt++;
            //Receive a reply from the server
            if( recv(sock, reply, MAX_MSG_LEN, 0) < 0) error("recv failed");
            char rcmd;
            int rpara;
            //printf("[Debug]:%s",reply);
            int pc=sscanf(reply,"%c%d\n",&rcmd,&rpara);            
            if (pc!=2||rcmd!='D') error("Invalid reply");
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
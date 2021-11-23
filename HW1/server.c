#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    int id;          //902001-902020
    int AZ;          
    int BNT;         
    int Moderna;     
}registerRecord;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
    registerRecord record;
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";
const char* operation_failed_header = "[Error] Operation failed. Please try again.\n";
const char* locked_header = "Locked.\n";
const char* enter_next_header = "Please input your preference order respectively(AZ,BNT,Moderna):\n";

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

int handle_read(request* reqP) {
    int r;
    char buf[512];
    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
    char* p1 = strstr(buf, "\015\012");
    int newline_len = 2;
    if (p1 == NULL) {
       p1 = strstr(buf, "\012");
        if (p1 == NULL) {
            ERR_EXIT("this really should not happen...");
        }
    }
    size_t len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len-1;
    return 1;
}

void connect_off(request* requestP, fd_set *master){
    FD_CLR(requestP->conn_fd, master);
    close(requestP->conn_fd);
    free_request(requestP);
    return;
}

int my_isnum(char *buf){
    for(int i = 0;i<strlen(buf);i++){
        if(!isdigit(buf[i])){
            return 0;
        }
    }
    return 1;
}

int onetwothree[6][3] = {{1,2,3},{1,3,2},{2,1,3},{2,3,1},{3,1,2},{3,2,1}};

int check_123(char *a, char *b, char *c){
    if(my_isnum(a) && my_isnum(b) && my_isnum(c)){
        int aa = atoi(a);
        int bb = atoi(b);
        int cc = atoi(c);
        for(int i = 0;i<6;i++){
            if(aa == onetwothree[i][0] && bb == onetwothree[i][1] && cc == onetwothree[i][2]){
                return 1;
            }
        }
    }
    return 0;
}

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512];
    int buf_len;

    fd_set master;
    fd_set read_fds;
    int write_lock[21] = {0};
    char vac_1[512];
    char vac_2[512];
    char vac_3[512];

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // read record
    file_fd = open("./registerRecord", O_RDWR);
    FD_ZERO(&master);
    // add server fd to master
    FD_SET(svr.listen_fd, &master);
    //lock
    struct flock temp_file_lock;

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    while (1) {
        // TODO: Add IO multiplexing
        
        // dup master
        read_fds = master;

        // select
        if(select(maxfd, &read_fds, NULL, NULL, NULL) < 0){
            ERR_EXIT("select");
        }

        for(int i = 0;i<maxfd;i++){
            if(FD_ISSET(i,&read_fds)){
                if(i == svr.listen_fd){
                     // Check new connection
                    clilen = sizeof(cliaddr);
                    conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
                    if (conn_fd < 0) {
                        if (errno == EINTR || errno == EAGAIN) continue;  // try again
                        if (errno == ENFILE) {
                            (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                            continue;
                        }
                        ERR_EXIT("accept");
                    }
                    requestP[conn_fd].conn_fd = conn_fd;
                    strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
                    fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
                    FD_SET(conn_fd, &master);
                    sprintf(buf, "Please enter your id (to check your preference order):\n");
                    write(conn_fd, buf, strlen(buf));
                }else{
                    int ret = handle_read(&requestP[i]); // parse data from client to requestP[conn_fd].buf
                    if (ret < 0) {
                        fprintf(stderr, "bad request from %s\n", requestP[conn_fd].host);
                        connect_off(&requestP[i], &master);
                        continue;
                    }

    // TODO: handle requests from clients
#ifdef READ_SERVER    
                    if(requestP[i].buf_len != 6){
                        write(requestP[i].conn_fd, operation_failed_header, strlen(operation_failed_header));
                        connect_off(&requestP[i], &master);
                        break;
                    }
                    requestP[i].id = atoi(requestP[i].buf);
                    //get lock info
                    temp_file_lock.l_type = F_RDLCK;
                    temp_file_lock.l_whence = SEEK_SET;
                    temp_file_lock.l_start = (requestP[i].id-902001)*4*sizeof(int);
                    temp_file_lock.l_len = 4*sizeof(int);

                    //check lock
                    if(requestP[i].id > 902020 || requestP[i].id < 902001){
                        write(requestP[i].conn_fd, operation_failed_header, strlen(operation_failed_header));
                        connect_off(&requestP[i], &master);
                        break;
                    }
                    if(fcntl(file_fd, F_SETLK, &temp_file_lock) < 0){
                        //locked
                        write(requestP[i].conn_fd, locked_header, strlen(locked_header));
                    }else{
                        //output read lines
                        lseek(file_fd, (requestP[i].id-902001)*4*sizeof(int), SEEK_SET);
                        int r1 = read(file_fd, &(requestP[i].record.id), sizeof(int));
                        int r2 = read(file_fd, &(requestP[i].record.AZ), sizeof(int));
                        int r3 = read(file_fd, &(requestP[i].record.BNT), sizeof(int));
                        int r4 = read(file_fd, &(requestP[i].record.Moderna), sizeof(int));

                        // release lock
                        temp_file_lock.l_type = F_UNLCK;
                        temp_file_lock.l_whence = SEEK_SET;
                        temp_file_lock.l_start = (requestP[i].id-902001)*4*sizeof(int);
                        temp_file_lock.l_len = 4*sizeof(int);
                        if(fcntl(file_fd, F_SETLK, &temp_file_lock) < 0){
                            ERR_EXIT("fcntl");
                        }

                        if(requestP[i].record.AZ == 1){
                            if(requestP[i].record.BNT == 2){
                                sprintf(buf, "Your preference order is AZ > BNT > Moderna.\n");
                            }else{
                                sprintf(buf, "Your preference order is AZ > Moderna > BNT.\n");
                            }
                        }else if(requestP[i].record.AZ == 2){
                            if(requestP[i].record.BNT == 1){
                                sprintf(buf, "Your preference order is BNT > AZ > Moderna.\n");
                            }else{
                                sprintf(buf, "Your preference order is Moderna > AZ > BNT.\n");
                            }
                        }else{
                            if(requestP[i].record.BNT == 1){
                                sprintf(buf, "Your preference order is BNT > Moderna > AZ.\n");
                            }else{
                                sprintf(buf, "Your preference order is Moderna > BNT > AZ.\n");
                            }
                        }
                        write(requestP[i].conn_fd, buf, strlen(buf));
                    }
                    connect_off(&requestP[i], &master);
                    break;
#elif defined WRITE_SERVER
                    if(requestP[i].wait_for_write == 0){
                        //first input
                        if(requestP[i].buf_len != 6){
                            write(requestP[i].conn_fd, operation_failed_header, strlen(operation_failed_header));
                            connect_off(&requestP[i], &master);
                            break;
                        }
                        requestP[i].id = atoi(requestP[i].buf);
                        if(requestP[i].id > 902020 || requestP[i].id < 902001){
                            write(requestP[i].conn_fd, operation_failed_header, strlen(operation_failed_header));
                            connect_off(&requestP[i], &master);
                            break;
                        }

                        //get lock info
                        temp_file_lock.l_type = F_WRLCK;
                        temp_file_lock.l_whence = SEEK_SET;
                        temp_file_lock.l_start = (requestP[i].id - 902001)*4*sizeof(int);
                        temp_file_lock.l_len = 4*sizeof(int);

                        if(fcntl(file_fd, F_SETLK, &temp_file_lock) < 0){
                            //locked
                            write(requestP[i].conn_fd, locked_header, strlen(locked_header));
                            connect_off(&requestP[i], &master);
                            break;
                        }
                        if(write_lock[requestP[i].id-902000] == 1){
                            write(requestP[i].conn_fd, locked_header, strlen(locked_header));
                            connect_off(&requestP[i], &master);
                            break;
                        }

                        //set local write lock
                        write_lock[requestP[i].id-902000] = 1;
                        requestP[i].wait_for_write = 1;
                        
                        //read
                        lseek(file_fd, (requestP[i].id-902001)*4*sizeof(int), SEEK_SET);
                        int r1 = read(file_fd, &(requestP[i].record.id), sizeof(int));
                        int r2 = read(file_fd, &(requestP[i].record.AZ), sizeof(int));
                        int r3 = read(file_fd, &(requestP[i].record.BNT), sizeof(int));
                        int r4 = read(file_fd, &(requestP[i].record.Moderna), sizeof(int));
                        if(requestP[i].record.AZ == 1){
                            if(requestP[i].record.BNT == 2){
                                sprintf(buf, "Your preference order is AZ > BNT > Moderna.\n");
                            }else{
                                sprintf(buf, "Your preference order is AZ > Moderna > BNT.\n");
                            }
                        }else if(requestP[i].record.AZ == 2){
                            if(requestP[i].record.BNT == 1){
                                sprintf(buf, "Your preference order is BNT > AZ > Moderna.\n");
                            }else{
                                sprintf(buf, "Your preference order is Moderna > AZ > BNT.\n");
                            }
                        }else{
                            if(requestP[i].record.BNT == 1){
                                sprintf(buf, "Your preference order is BNT > Moderna > AZ.\n");
                            }else{
                                sprintf(buf, "Your preference order is Moderna > BNT > AZ.\n");
                            }
                        }
                        write(requestP[i].conn_fd, buf, strlen(buf));
                        write(requestP[i].conn_fd, enter_next_header, strlen(enter_next_header));
                        break;
                    }else{
                        //second input
                        char str1,str2,str3,str4,str5;
                        if(requestP[i].buf_len != 5){
                            write(requestP[i].conn_fd, operation_failed_header, strlen(operation_failed_header));
                        }else{
                            int num_of_sscanf = sscanf(requestP[i].buf, "%s%s%s", vac_1, vac_2, vac_3);
                            if(num_of_sscanf == 3){
                                if(check_123(vac_1, vac_2, vac_3) == 1){
                                    requestP[i].record.AZ = atoi(vac_1);
                                    requestP[i].record.BNT = atoi(vac_2);
                                    requestP[i].record.Moderna = atoi(vac_3);
                                    lseek(file_fd, ((requestP[i].id-902000)*4-3)*sizeof(int), SEEK_SET);
                                    write(file_fd, &(requestP[i].record.AZ), sizeof(int));
                                    write(file_fd, &(requestP[i].record.BNT), sizeof(int));
                                    write(file_fd, &(requestP[i].record.Moderna), sizeof(int));
                                    if(requestP[i].record.AZ == 1){
                                        if(requestP[i].record.BNT == 2){
                                            sprintf(buf, "Preference order for %d modified successed, new preference order is AZ > BNT > Moderna.\n", requestP[i].record.id);
                                        }else{
                                            sprintf(buf, "Preference order for %d modified successed, new preference order is AZ > Moderna > BNT.\n", requestP[i].record.id);
                                        }
                                    }else if(requestP[i].record.AZ == 2){
                                        if(requestP[i].record.BNT == 1){
                                            sprintf(buf, "Preference order for %d modified successed, new preference order is BNT > AZ > Moderna.\n", requestP[i].record.id);
                                        }else{
                                            sprintf(buf, "Preference order for %d modified successed, new preference order is Moderna > AZ > BNT.\n", requestP[i].record.id);
                                        }
                                    }else{
                                        if(requestP[i].record.BNT == 1){
                                            sprintf(buf, "Preference order for %d modified successed, new preference order is BNT > Moderna > AZ.\n", requestP[i].record.id);
                                        }else{
                                            sprintf(buf, "Preference order for %d modified successed, new preference order is Moderna > BNT > AZ.\n", requestP[i].record.id);
                                        }
                                    }
                                    write(requestP[i].conn_fd, buf, strlen(buf));
                                }else{
                                    write(requestP[i].conn_fd, operation_failed_header, strlen(operation_failed_header));
                                }
                            }else{
                                write(requestP[i].conn_fd, operation_failed_header, strlen(operation_failed_header));
                            }
                        }
                        //release lock
                        write_lock[requestP[i].id-902000] = 0;
                        temp_file_lock.l_type = F_UNLCK;
                        temp_file_lock.l_whence = SEEK_SET;
                        temp_file_lock.l_start = (requestP[i].id-902001)*4*sizeof(int);
                        temp_file_lock.l_len = 4*sizeof(int);
                        if(fcntl(file_fd, F_SETLK, &temp_file_lock) < 0){
                            ERR_EXIT("fcntl");
                        }
                        connect_off(&requestP[i], &master);
                        break;
                    }
#endif
                }
            }
        }
    }
    close(file_fd);
    free(requestP);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
    reqP->wait_for_write = 0;
    reqP->record.AZ = 0;
    reqP->record.Moderna = 0;
    reqP->record.BNT = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/stat.h>

int main(int argc, char **argv){
    if(argc != 7){
        fprintf(stderr, "%s: usage : %s [player_id]\n", argv[0], argv[0]);
        exit(1);
    }
    // get arguments
    int host_id;
    int depth;
    int lucky_number;
    char c;
    while((c=getopt(argc, argv, "m:d:l:")) != -1){
        switch(c){
            case 'm':
                host_id = atoi(optarg);
                break;
            case 'd':
                depth = atoi(optarg);
                break;
            case 'l':
                lucky_number = atoi(optarg);
                break;
        }
    }
    char buf[512];
    int status;
    if(depth == 0){ // parent
        sprintf(buf, "fifo_%d.tmp", host_id);
        FILE *recv_fifo = fopen(buf, "r+");
        FILE *send_fifo = fopen("fifo_0.tmp", "r+");
        int player_ID[8];
        // make pipe
        pid_t pid1, pid2;
        int pipe1_recv[2], pipe2_recv[2], pipe1_send[2], pipe2_send[2];
        pipe(pipe1_recv);pipe(pipe2_recv);pipe(pipe1_send);pipe(pipe2_send);
        // fork
        if((pid1 = fork()) && (pid2 = fork()) < 0){
            fprintf(stderr, "fork error.\n", 12);
        }
        if(pid1 == 0){ // left child
            dup2(pipe1_send[0], STDIN_FILENO);
            close(pipe1_send[1]);
            dup2(pipe1_recv[1], STDOUT_FILENO);
            close(pipe1_recv[0]);
            close(pipe2_recv[0]);close(pipe2_recv[1]);
            close(pipe2_send[0]);close(pipe2_send[1]);
            char hid[512], lckn[512];
            sprintf(hid, "%d", host_id);
            sprintf(lckn, "%d", lucky_number);
            execlp("./host", "./host", "-m", hid, "-d", "1", "-l", lckn, NULL);
        }else if(pid2 == 0){ // right child
            dup2(pipe2_send[0], STDIN_FILENO);
            close(pipe2_send[1]);
            dup2(pipe2_recv[1], STDOUT_FILENO);
            close(pipe2_send[0]);
            close(pipe1_recv[0]);close(pipe1_recv[1]);
            close(pipe1_send[0]);close(pipe1_send[1]);
            char hid[512], lckn[512];
            sprintf(hid, "%d", host_id);
            sprintf(lckn, "%d", lucky_number); 
            execlp("./host", "./host",  "-m", hid, "-d", "1", "-l", lckn, NULL);
        }else{ // parent
            FILE *fp1_send = fdopen(pipe1_send[1], "w");
            FILE *fp1_recv = fdopen(pipe1_recv[0], "r");
            FILE *fp2_send = fdopen(pipe2_send[1], "w");
            FILE *fp2_recv = fdopen(pipe2_recv[0], "r");
            close(pipe1_send[0]);close(pipe1_recv[1]);close(pipe2_send[0]);close(pipe2_recv[1]);
            fscanf(recv_fifo, "%d %d %d %d %d %d %d %d", &player_ID[0], &player_ID[1], &player_ID[2], &player_ID[3], &player_ID[4], &player_ID[5], &player_ID[6], &player_ID[7]);
            
            while(player_ID[0] != -1){ // keep parent alive
                fprintf(fp1_send, "%d %d %d %d\n", player_ID[0], player_ID[1], player_ID[2], player_ID[3]);
                fflush(fp1_send);
                fprintf(fp2_send, "%d %d %d %d\n", player_ID[4], player_ID[5], player_ID[6], player_ID[7]);
                fflush(fp2_send);
                // calculate point
                int point[13] = {0};
                for(int i = 0;i<10;i++){
                    int id1, id2, guess1, guess2;
                    fscanf(fp1_recv, "%d %d", &id1, &guess1);
                    fscanf(fp2_recv, "%d %d", &id2, &guess2);
                    if(abs(lucky_number-guess1) <= abs(lucky_number-guess2)){
                        point[id1] += 10; 
                    }else{
                        point[id2] += 10;
                    }
                }

                sprintf(buf, "%d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n", host_id, player_ID[0], point[player_ID[0]], player_ID[1], point[player_ID[1]], player_ID[2], point[player_ID[2]], player_ID[3], point[player_ID[3]], player_ID[4], point[player_ID[4]], player_ID[5], point[player_ID[5]], player_ID[6], point[player_ID[6]], player_ID[7], point[player_ID[7]]);
                fprintf(send_fifo, buf, strlen(buf));
                fflush(send_fifo); 
                // write block
                fscanf(recv_fifo, "%d %d %d %d %d %d %d %d", &player_ID[0], &player_ID[1], &player_ID[2], &player_ID[3], &player_ID[4], &player_ID[5], &player_ID[6], &player_ID[7]);
            }
            // get -1 then end
            fprintf(fp1_send, "%d %d %d %d\n", player_ID[0], player_ID[1], player_ID[2], player_ID[3]);
            fflush(fp1_send);
            fprintf(fp2_send, "%d %d %d %d\n", player_ID[4], player_ID[5], player_ID[6], player_ID[7]);
            fflush(fp2_send);
            waitpid(pid1, &status, 0);
            waitpid(pid2, &status, 0);
            fclose(fp1_send);fclose(fp1_recv);
            fclose(fp2_send);fclose(fp2_recv);
        }
        exit(0);
    }else if(depth == 1){
        int player_ID[4];
        for(int i = 0;i<4;i++){
            scanf("%d", &player_ID[i]);
        }
        // make pipe
        pid_t pid1, pid2;
        int pipe1_recv[2], pipe2_recv[2], pipe1_send[2], pipe2_send[2];
        pipe(pipe1_recv);pipe(pipe2_recv);pipe(pipe1_send);pipe(pipe2_send);
        // fork
        if((pid1 = fork()) && (pid2 = fork()) < 0){
            fprintf(stderr, "fork error.\n", 12);
        }
        if(pid1 == 0){ // left leaf
            dup2(pipe1_send[0], STDIN_FILENO);
            close(pipe1_send[1]);
            dup2(pipe1_recv[1], STDOUT_FILENO);
            close(pipe1_recv[0]);
            close(pipe2_recv[0]);close(pipe2_recv[1]);
            close(pipe2_send[0]);close(pipe2_send[1]);
            char hid[512], lckn[512];
            sprintf(hid, "%d", host_id);
            sprintf(lckn, "%d", lucky_number);
            execlp("./host", "./host",  "-m", hid, "-d", "2", "-l", lckn, NULL);
        }else if(pid2 == 0){ // right leaf
            dup2(pipe2_send[0], STDIN_FILENO);
            close(pipe2_send[1]);
            dup2(pipe2_recv[1], STDOUT_FILENO);
            close(pipe2_send[0]);
            close(pipe1_recv[0]);close(pipe1_recv[1]);
            close(pipe1_send[0]);close(pipe1_send[1]);
            char hid[512], lckn[512];
            sprintf(hid, "%d", host_id);
            sprintf(lckn, "%d", lucky_number);     
            execlp("./host", "./host",  "-m", hid, "-d", "2", "-l", lckn, NULL);
        }else{ // child
            FILE *fp1_send = fdopen(pipe1_send[1], "w");
            FILE *fp1_recv = fdopen(pipe1_recv[0], "r");
            FILE *fp2_send = fdopen(pipe2_send[1], "w");
            FILE *fp2_recv = fdopen(pipe2_recv[0], "r");
            close(pipe1_send[0]);close(pipe1_recv[1]);close(pipe2_send[0]);close(pipe2_recv[1]);
            
            while(player_ID[0] != -1){ // keep child alive
                fprintf(fp1_send, "%d %d\n", player_ID[0], player_ID[1]);
                fflush(fp1_send);
                fprintf(fp2_send, "%d %d\n", player_ID[2], player_ID[3]);
                fflush(fp2_send);
                // send closest id and guess number to parent
                for(int i = 0;i<10;i++){
                    int id1, id2, guess1, guess2;
                    fscanf(fp1_recv, "%d%d", &id1, &guess1);
                    fscanf(fp2_recv, "%d%d", &id2, &guess2);
                    if(abs(lucky_number-guess1) <= abs(lucky_number-guess2)){
                        printf("%d %d\n", id1, guess1);
                        fflush(stdout);
                    }else{
                        printf("%d %d\n", id2, guess2);
                        fflush(stdout);
                    }
                }
                // write block
                for(int i = 0;i<4;i++){
                    scanf("%d", &player_ID[i]);
                }
            }
            // get -1 then end
            fprintf(fp1_send, "%d %d\n", player_ID[0], player_ID[1]);
            fflush(fp1_send);
            fprintf(fp2_send, "%d %d\n", player_ID[2], player_ID[3]);
            fflush(fp2_send);
            waitpid(pid1, &status, 0);
            waitpid(pid2, &status, 0);
            fclose(fp1_send);fclose(fp1_recv);
            fclose(fp2_send);fclose(fp2_recv);
        }
        exit(0);
    }else if(depth == 2){ // leaf
        int player_ID[2];
        for(int i = 0;i<2;i++){
            scanf("%d", &player_ID[i]);
        }
        while(player_ID[0] != -1){ // keep leaf alive
            // make pipe
            pid_t pid1, pid2;
            int pipe1_recv[2], pipe2_recv[2];
            pipe(pipe1_recv);pipe(pipe2_recv);
            // fork
            if((pid1 = fork()) && (pid2 = fork()) < 0){
                fprintf(stderr, "fork error.\n", 12);
            }
            if(pid1 == 0){ // left player
                dup2(pipe1_recv[1], STDOUT_FILENO);
                close(pipe1_recv[0]);
                close(pipe2_recv[0]);close(pipe2_recv[1]);
                sprintf(buf, "%d", player_ID[0]);
                execlp("./player", "./player", "-n", buf, NULL);
            }else if(pid2 == 0){ // right player
                dup2(pipe2_recv[1], STDOUT_FILENO);
                close(pipe2_recv[0]);
                close(pipe1_recv[0]);close(pipe1_recv[1]);
                sprintf(buf, "%d", player_ID[1]);
                execlp("./player", "./player", "-n", buf, NULL);
            }else{ // leaf
                FILE *fp1_recv = fdopen(pipe1_recv[0], "r");            
                FILE *fp2_recv = fdopen(pipe2_recv[0], "r");
                close(pipe1_recv[1]);close(pipe2_recv[1]);
                for(int i = 0;i<10;i++){
                    // send closest id and guess number to child
                    int id1, id2, guess1, guess2;
                    fscanf(fp1_recv, "%d%d", &id1, &guess1);
                    fscanf(fp2_recv, "%d%d", &id2, &guess2);
                    if(abs(lucky_number-guess1) <= abs(lucky_number-guess2)){
                        printf("%d %d\n", id1, guess1);
                        fflush(stdout);
                        fsync(STDOUT_FILENO);
                    }else{
                        printf("%d %d\n", id2, guess2);
                        fflush(stdout);
                        fsync(STDOUT_FILENO);
                    }
                }
                fclose(fp1_recv);fclose(fp2_recv);
                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);
                // write block
                for(int i = 0;i<2;i++){
                    scanf("%d", &player_ID[i]);
                }
            }
        }
        // get -1 then end
        exit(0);
    }
    return 0;
}
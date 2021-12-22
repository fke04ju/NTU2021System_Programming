#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
    int player_id;
    int guess;
    if(argc != 3){
        fprintf(stderr, "%s: usage : %s [player_id]\n", argv[0], argv[0]);
        exit(1);
    }
    player_id = atoi(argv[2]);
    for(int i = 1;i<=10;i++){
        srand((player_id+i)*323);
        guess = rand() % 1001;
        printf("%d %d\n", player_id, guess);
        fflush(stdout);
    }
    exit(0);
}
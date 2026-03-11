#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s keylength\n", argv[0]);
        exit(1);
    }

    int length = atoi(argv[1]);
    srand(time(0)); // seed the random number generator

    char allowed[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
    
    for (int i = 0; i < length; i++) {
        // pick a random index from 0 to 26
        printf("%c", allowed[rand() % 27]);
    }
    
    // keygen end with newline
    printf("\n");

    return 0;
}

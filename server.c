#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>

void *handlePlayer(void *arg);

struct PlayerData {
    int client;
    int* ownScorePtr;
    int* opponentScorePtr;
    pthread_mutex_t* scoreMutexPtr;
    int* gameEndPtr;
    int* wonGamePtr;
};

int main(int argc, char *argv[]) {
    int sd, client1, client2, portNumber;
    socklen_t len;
    struct sockaddr_in servAdd;

    if (argc != 2) {
        fprintf(stderr, "Call model: %s <Port#>\n", argv[0]);
        exit(0);
    }
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "[-] Could not create socket\n");
        exit(1);
    } else {
        fprintf(stderr, "[+] Socket Created\n");
    }
    servAdd.sin_family = AF_INET;
    servAdd.sin_addr.s_addr = htonl(INADDR_ANY);
    sscanf(argv[1], "%d", &portNumber);
    servAdd.sin_port = htons((uint16_t) portNumber);
    bind(sd, (struct sockaddr *) &servAdd, sizeof(servAdd));

    if (listen(sd, 6) == 0) {
        printf("[+] Listening...\n");
    } else {
        printf("[-] Error in binding.\n");
    }

    pthread_mutex_t score_mutex;
    pthread_mutex_init(&score_mutex, NULL);

    while (1) {
        client1 = accept(sd, (struct sockaddr *) NULL, NULL);
        write(client1, "Waiting for player 2 to join ...", 100);
        client2 = accept(sd, (struct sockaddr *) NULL, NULL);
        write(client1, "Player 2 joined ... Game is Starting ...", 100);
        write(client2, "Player 1 already joined ... Game is Starting ...", 100);
        printf("[+] Got a game request\n");

        int player1Score = 0;
        int player2Score = 0;
        int gameEnd = 0;
        int player1Won = 0;
        int player2Won = 0;

        struct PlayerData player1Data = {client1, &player1Score, &player2Score, &score_mutex, &gameEnd, &player1Won};
        struct PlayerData player2Data = {client2, &player2Score, &player1Score, &score_mutex, &gameEnd, &player2Won};

        pthread_t thread1, thread2;
        pthread_create(&thread1, NULL, handlePlayer, (void *) &player1Data);
        pthread_create(&thread2, NULL, handlePlayer, (void *) &player2Data);

        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);

        // Check if either player has won
        if (player1Won) {
            write(client1, "Game over: You won the game", 100);
            write(client2, "Game over: Opponent won the game", 100);
        } else if (player2Won) {
            write(client2, "Game over: You won the game", 100);
            write(client1, "Game over: Opponent won the game", 100);
        }

        close(client1);
        close(client2);
    }

    pthread_mutex_destroy(&score_mutex);
    return 0;
}

void *handlePlayer(void *arg) {
    struct PlayerData* playerData = (struct PlayerData*) arg;
    int client = playerData->client;
    int* ownScorePtr = playerData->ownScorePtr;
    int* opponentScorePtr = playerData->opponentScorePtr;
    pthread_mutex_t* scoreMutexPtr = playerData->scoreMutexPtr;
    int* gameEndPtr = playerData->gameEndPtr;
    int* wonGamePtr = playerData->wonGamePtr;

    char buf[100];
    int32_t conv_points;
    int size = sizeof(conv_points);

    while (1) {
        sleep(1);
        write(client, "You can now play", 100);
        if (read(client, &conv_points, size) < 0) {
            printf("Read Error from Player\n");
            break;
        }

        int points = ntohl(conv_points);
        pthread_mutex_lock(scoreMutexPtr);
        *ownScorePtr += points;
        snprintf(buf, 100, "Your Score is :: %d\nOpponent's Score is :: %d\n", *ownScorePtr, *opponentScorePtr);
        write(client, buf, 100);

        if (*ownScorePtr >= 100) {
            if (!*wonGamePtr) {
                *wonGamePtr = 1;
                *gameEndPtr = 1;
                
            }
        } else if (*opponentScorePtr >= 100) {
            if (!*wonGamePtr) {
                *wonGamePtr = 1;
                *gameEndPtr = 1;
                
            }
        }
        
        pthread_mutex_unlock(scoreMutexPtr);

        if (*gameEndPtr) {
            break; 
        }
    }

    pthread_exit(NULL);
}
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>
#include <stdint.h>

#include "cards.h"

#define PORT_NO 5000
#define MAX_PLAYERS 1000
#define QLEN 5

unsigned int activeconn = 0;
unsigned long totalconn = 0;
int victories[3];
FILE *logfp;

void shuffle(deck_t *deck);
void playgame(void *newsock);
void updatehands_server(hand_t *hands, deck_t *deck, int *hit);
int start(int fd, hand_t *hands, deck_t *deck, int *hit);
// void log();

pthread_mutex_t count_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t active_mut = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char *argv[])
{
    int msock, newsock;
    int con;
    struct sockaddr_in serv_addr;
    struct timeval timeout;
    logfp = fopen("serverlog.txt", "a");
    if(logfp == NULL)  {
	printf("Open Log failed with errno %d\n\n", errno);
	return -1;
    }
    fprintf(logfp, "\nServer restarted.\n\n");
    fflush(logfp);
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    memset(&victories, 0, sizeof(int)*3);
    // thread log_thread;
    // pthread_create(&log_thread, NULL, &log, NULL);
    
    //for(i = 0; i < MAX_PLAYERS; i++)
    //	    player_sockets[i] = 0; 

    msock = socket(AF_INET, SOCK_STREAM, 0);
    if(msock < 0)
	    errexit("Error creating socket\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT_NO);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(msock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	    errexit("Bind error\n");

    listen(msock, QLEN);
    
    while(1)
    {
	pthread_mutex_lock(&active_mut);
	while(activeconn > MAX_PLAYERS)	{ // Server is full
		pthread_mutex_unlock(&active_mut);
		sleep(5);
		pthread_mutex_lock(&active_mut);
	}
	pthread_mutex_unlock(&active_mut);
	newsock = accept(msock, (struct sockaddr*)NULL, NULL);

	if (newsock < 0) {
		printf("accept failed\n");
		sleep(1);
		continue;
	}

	setsockopt(newsock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
	pthread_mutex_lock(&active_mut);
	activeconn++;
	con=activeconn;
	pthread_mutex_unlock(&active_mut);
	pthread_mutex_lock(&log_mut);
	fprintf(logfp,"There are currently %d active players.\n", con);
	fflush(logfp);
	pthread_mutex_unlock(&log_mut);
	/* for(i = 0; i < MAX_PLAYERS; i++)   {
		if(player_sockets[i] == 0)	{
			player_sockets[i] = newsock;
			break;
		}
	}*/
	pthread_t t;
	pthread_create(&t, NULL, (void *) &playgame, (void *)&newsock);
    }
}

void shuffle(deck_t *deck)	{

	int i, j, temp;
	time_t t;

	for(i = 0; i < DECKSIZE;i++)
		deck->cards[i] = i;

	// Shuffle unshuffled deck using Fisher-Yates Shuffle

	srand ((unsigned) time (&t));

	// shuffle deck; iterate downward through deck, move card at location i to random location less than i.
	for(i = DECKSIZE - 1; i > 0; i--)	{
		j = rand() % i;

		temp = deck->cards[j];
		deck->cards[j] = deck->cards[i];
		deck->cards[i] = temp;
	}
}

#define SIZE_HIT	((MAX_PLAYERS_PER_GAME) * sizeof(int))

int start(int fd, hand_t *hands, deck_t *deck, int *hit)
{
	int i, res = -1;

	for(i = 0; i < MAX_PLAYERS_PER_GAME; i++)
	{
		hit[i] = 1;
		memset(&hands[i], 0, sizeof(hand_t));
	}
	// hit[MAX_PLAYERS_PER_GAME] = 0; // When this is 1, that is the message to the client that the game is over.

	shuffle(deck);
	updatehands_server(hands, deck, hit);
	// for(i=0; i < num - 1; i++)
	res = write(fd, hit, SIZE_HIT);
	if (res < SIZE_HIT) {
		printf("Write failed with errno %d\n", errno);
		res = -1;
		goto exit;
	}
	hit[0] = -1; // Dealer starts with only 1 card revealed
	for(i = 1; i < MAX_PLAYERS_PER_GAME; i++)
		hit[i] = 1;
	updatehands_server(hands, deck, hit); // Create initial hand
	// for(i=0; i < num - 1; i++)
	res = write(fd, hit, SIZE_HIT);
	if (res < SIZE_HIT) {
		printf("Write failed with errno %d\n", errno);
		res = -1;
		goto exit;
	}
exit:
	return res;
}

// Function called at beginning of game
void playgame(void *newsock)	{

	pthread_detach(pthread_self());
	pthread_mutex_lock(&count_mut);
	totalconn++;
	long id = totalconn;
	pthread_mutex_unlock(&count_mut);
	pthread_mutex_lock(&log_mut);
	fprintf(logfp, "Client %ld began a new game.\n", id);
	fflush(logfp);
	pthread_mutex_unlock(&log_mut);

	int fd = *(int *)newsock;
	deck_t *deck = (deck_t *)malloc(sizeof(*deck));
	deck->len = DECKSIZE;
	hand_t hands[MAX_PLAYERS_PER_GAME]; // Element 0 is the dealer, other players are higher numbers 
	int done = 0, res; 
	int hit[MAX_PLAYERS_PER_GAME];

	assert(hit!=NULL);

	res = start(fd, hands, deck, hit);
	if(res == -1)
		goto exit;

	// Actually play the game
	while(!done)
	{
		res = read(fd, &hit[1], sizeof(int));
		if(res <= 0)	{
			goto exit;
		}
		if(hit[1] == 2) // expand to more players in future
			done = 1; // All players have finished
		else
		{
			updatehands_server(hands, deck, hit);
			res = write(fd, hit, sizeof(hit));
			if (res < sizeof(hit)) {
				printf("Write failed with errno %d\n", errno);
				res = -1;
				goto exit;
			}
			if(hands[1].value > 21) { // Player busted
				hit[1] = -1;
				done = 1;
			}
		}
	}
	// Server plays now
	int update[10], i;
	// All of the server's moves will be sent as one update; the first element of the 
	// array will say how many updates there are, and the rest will be the updates.
	for(i = 1; i < 10; i++)	{
		hit[0] = 1;
		updatehands_server(hands, deck, hit);

		if(hands[0].value >= 17)  {
			update[0] = i;
			break;
		}
		update[i] = hit[0];
	}
	res = write(fd, update, sizeof(update));
	if (res < sizeof(update)) {
		printf("Write failed with errno %d\n", errno);
		res = -1;
	}
exit:
	free(deck);
	close(fd);
	int result = score(hands);
	if(result < 0 || result > 2)
		errexit("Score error.\n");
	pthread_mutex_lock(&active_mut);
	activeconn--;
	pthread_mutex_unlock(&active_mut);
	pthread_mutex_lock(&log_mut);
	victories[result]++;
	fprintf(logfp, "Client %ld finished.\n", id);
	fprintf(logfp, "In this run of the server, the dealer has won %d times, " 
			"lost %d times, and drew %d times.\n", victories[0], victories[1], victories[2]);
	fflush(logfp);
	pthread_mutex_unlock(&log_mut);
	pthread_exit(NULL);
}

void updatehands_server(hand_t *hands, deck_t *deck, int *hit)
{
	int i, card, check;

	for(i = 0; i < MAX_PLAYERS_PER_GAME; i++)
	{
		// printf("len=%d\n", deck->len);
		assert(deck->len>=0);
		if(hit[i] == 1)
		{
			deck->len--;
			card = deck->cards[deck->len];
			deck->cards[deck->len] = -1; // Not necessary, but nice to see if something odd is happening
			check = newhandval(&hands[i], card);
			if(check != 0)  {
				pthread_exit(NULL);
				errexit("Attempted to access nonexistent card.\n");
			}
			hit[i] = card;
		}
		else
			hit[i] = -1;
	}
}


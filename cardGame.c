#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#define PLAYERS 3
#define MAX_HAND 2
#define ROUNDS 3
#define CARD_COUNT 52

// Cards and Hands
struct card{
	int val;
	struct card *next;
};
struct card *topPtr = NULL;
struct card *temPtr = NULL;
struct card *bacPtr = NULL;
struct card hands[PLAYERS][MAX_HAND];

// Synch Prep
pthread_t threads[PLAYERS + 1];
// Current Round
pthread_mutex_t curRoundMut;
pthread_cond_t curRoundCond;
int curRound = 0;
// Deck access.
pthread_mutex_t deckMut;
// Draw access.
pthread_mutex_t drawMut;
pthread_cond_t drawCond;
int draw;
// Table synch
pthread_mutex_t tableMut;
pthread_cond_t tableCond;
int table;
// File access
pthread_mutex_t fileMut;
FILE *filePtr;

// Global Vars
int rc, hc, dc = 0, randCard;
bool victor = false;

void genDeck();
void freeDeck();	
void printDeck();
void shufDeck();
int getHandCount();
struct card popDeck();
void returnCards(intptr_t tid);
void printHand(intptr_t tid);
void pushDeck(struct card pushCard, intptr_t tid);
void pushHand(struct card pushCard, intptr_t tid);
void waitForTable(intptr_t tid, int bar);
void logHand(intptr_t tid);
void logDeck();
void endRound();
void countDeck();
void *dealer(void *id);
void *player(void *id);

int main(int argc, char *argv[]){
	printf("\nWelcome to the Card Game\n");

	// Main prep.
	int randSeed;
	if (argc != 2){
		printf("\nERROR. Please include seed integer in command line.");
		printf("\nEx)  gcc prj_2.c -o prj_2 -lpthread -std=c99");
		printf("\n     ./prj_2 8\n\n");
		exit(-1);
	}
	else{
		randSeed = atoi(argv[1]);
	}
	srand(randSeed);

	filePtr = fopen("log.txt", "w");
	if (filePtr == NULL){
		printf("\nERROR!  File log.txt failed to open.");
		exit(-1);
	}

	// Thread prep.
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	// Synch prep
	pthread_cond_init(&curRoundCond, NULL);
	pthread_mutex_init(&curRoundMut, NULL);
	pthread_mutex_init(&deckMut, NULL);
	pthread_mutex_init(&drawMut, NULL);
	pthread_cond_init(&drawCond, NULL);
	pthread_mutex_init(&tableMut, NULL);
	pthread_cond_init(&tableCond, NULL);

	// Init hands.
	for (int j = 0; j < PLAYERS; j++){
		for (int k = 0; k < MAX_HAND; k++){
			hands[j][k].val = -1;
		}
	}

	// Create threads.
	for (intptr_t j = 0; j < PLAYERS + 1; j++){
		if (j == PLAYERS){
			// Create dealer thread.
			rc = pthread_create(&threads[j], &attr, dealer, (void*)j);
		}
		else{
			// Create player thread.
			rc = pthread_create(&threads[j], &attr, player, (void*)j);
		}
		if (rc){
			printf("\nERROR from dealer thread with return code %d.", rc);
			exit(-1);
		}	
	}
	pthread_attr_destroy(&attr);
 
	// Join threads.
	for (int z = 0; z < PLAYERS + 1; z++){
		rc = pthread_join(threads[z], NULL);
		if (rc){
			printf("\nERROR from thread %d joining main.", z);
			exit(-1);
		}
		//printf("\nThread %d has joined main.", z);
	}

	// Garbage collection.
	printf("\nGoodbye\n\n");
	fclose(filePtr);
	pthread_mutex_destroy(&deckMut);
	pthread_mutex_destroy(&fileMut);
	pthread_mutex_destroy(&curRoundMut);
	pthread_cond_destroy(&curRoundCond);
	pthread_mutex_destroy(&drawMut);
	pthread_cond_destroy(&drawCond);
	pthread_mutex_destroy(&tableMut);
	pthread_cond_destroy(&tableCond);
	freeDeck();
	free(topPtr);
	free(temPtr);
	free(bacPtr);
	pthread_exit(NULL);
}

void genDeck(){
	pthread_mutex_lock(&deckMut);
	//printf("\nDealer generates deck.");
	for (int i = 2; i <= 14; i++){
		for (int j = 0; j < 4; j++){
			temPtr = (struct card*)malloc(sizeof(struct card));
			temPtr->val = i;
			//temPtr->inDeck = true;
			temPtr->next = topPtr;
			topPtr = temPtr;
			//printCard(temPtr);
		}
	}
	temPtr = NULL;
	pthread_mutex_unlock(&deckMut);
}

void freeDeck(){
	pthread_mutex_lock(&deckMut);
	bacPtr = topPtr;
	while (bacPtr != NULL){
		temPtr = bacPtr->next;
		free(bacPtr);
		bacPtr = temPtr;
	}
	pthread_mutex_unlock(&deckMut);
}

void printDeck(){
	pthread_mutex_lock(&deckMut);
	int col = 0;
	temPtr = topPtr;
	printf("\n");
	while(temPtr != NULL){
		if (col % 4 == 0){printf("\n");}
		printf("[ %d ]\t", temPtr->val);
		temPtr = temPtr->next;
		col++;
	}
	printf("\n");
	temPtr = NULL;
	pthread_mutex_unlock(&deckMut);
}

void shufDeck(){
	//printf("\nDealer shuffles deck.");
	pthread_mutex_lock(&fileMut);
	fprintf(filePtr, "\nDEALER: shuffle deck");
	pthread_mutex_unlock(&fileMut);

	int x1, x2 = 0, shuf = 0;
	pthread_mutex_lock(&deckMut);
	while (shuf < 300){
		temPtr = topPtr;
		x1 = rand() % CARD_COUNT;
		//printf("\nx1: %d", x1);
		while (temPtr->next != NULL && x1 != x2){
			bacPtr = temPtr;
			temPtr = temPtr->next;
			x2++;
		}
		//printf("\nCard at index %d: %d", x2, temPtr->val);
		if (bacPtr != NULL){
			// Push card to top of deck.
			bacPtr->next = temPtr->next;
			temPtr->next = topPtr;
			topPtr = temPtr;
			bacPtr = NULL;
		}
		// Else do nothing - card already at top of deck.
		shuf++;	
		x2 = 0;	
	}
	temPtr = NULL;
	bacPtr = NULL;
	pthread_mutex_unlock(&deckMut);
}

void countDeck(){
	int count = 0;
	pthread_mutex_lock(&fileMut);
	fprintf(filePtr, "\nDEALER: count cards");
	pthread_mutex_unlock(&fileMut);

	pthread_mutex_lock(&deckMut);
	temPtr = topPtr;
	while(temPtr != NULL){
		count++;
		temPtr = temPtr->next;
	}
	temPtr = NULL;
	pthread_mutex_unlock(&deckMut);
	if (count != CARD_COUNT){
		printf("\nERROR! Not all cards in deck.");
		exit(-1);
	}
}

int getHandCount(){
	int count = 0;
	for (int i = 0; i < PLAYERS; i++){
		for (int k = 0; k < MAX_HAND; k++){
			if (hands[i][k].val != -1){
				count++;
			}
		}
	}
	return count;
}

void printHand(intptr_t tid){
	//printf("\nHand: ");
	for (int k = 0; k < MAX_HAND; k++){
		if (hands[tid][k].val != -1){
			printf("%d  ", hands[tid][k].val);
		}
	}
}

void logHand(intptr_t tid){
	pthread_mutex_lock(&fileMut);
	fprintf(filePtr, "\nPLAYER %ld: hand", tid+1);
	for (int k = 0; k < MAX_HAND; k++){
		if (hands[tid][k].val != -1){
			fprintf(filePtr, "  %d", hands[tid][k].val);
		}
	}
	pthread_mutex_unlock(&fileMut);
}

void logDraw(intptr_t tid, int x){
	pthread_mutex_lock(&fileMut);
	fprintf(filePtr, "\nPLAYER %ld: draw %d", tid+1, hands[tid][x].val);
	pthread_mutex_unlock(&fileMut);
}

void logDeck(){
	pthread_mutex_lock(&deckMut);
	pthread_mutex_lock(&fileMut);
	int col = 0;
	temPtr = topPtr;
	fprintf(filePtr, "\n");
	while(temPtr != NULL){
		if (col % 4 == 0){fprintf(filePtr, "\n");}
		fprintf(filePtr, "[ %d ]\t", temPtr->val);
		temPtr = temPtr->next;
		col++;
	}
	fprintf(filePtr, "\n");
	temPtr = NULL;
	pthread_mutex_unlock(&fileMut);
	pthread_mutex_unlock(&deckMut);
}

struct card popDeck(){
	pthread_mutex_lock(&deckMut);
	struct card popCard = *topPtr;
	topPtr = topPtr->next;
	popCard.next = NULL;
	//printf("\nPopDeck: %d", popCard.val);
	pthread_mutex_unlock(&deckMut);
	return popCard;
}

void pushDeck(struct card pushCard, intptr_t tid){
	
	pthread_mutex_lock(&fileMut);
	fprintf(filePtr, "\nPLAYER %ld: return %d", tid+1, pushCard.val);
	pthread_mutex_unlock(&fileMut);

	pthread_mutex_lock(&deckMut);
	//printf("\nReturn: %d", pushCard.val);
	temPtr = (struct card*)malloc(sizeof(struct card));
	*temPtr = pushCard;
	//printf("\ntemPtr val %d", temPtr->val);
	bacPtr = topPtr;
	while(bacPtr->next != NULL){
		bacPtr = bacPtr->next;
	}
	bacPtr->next = temPtr;
	temPtr->next = NULL;
	temPtr = NULL;
	pthread_mutex_unlock(&deckMut);
}

void returnCards(intptr_t tid){
	//printf("\nPlayer %ld returns card(s) to deck.", tid);
	for (int j = 0; j < MAX_HAND; j++){
		if (hands[tid][j].val != -1){
			pushDeck(hands[tid][j], tid);
			hands[tid][j].val = -1;
			hands[tid][j].next = NULL;
		}
	}
}

void pushHand(struct card pushCard, intptr_t tid){
	for (int j = 0; j < MAX_HAND; j++){
		if (hands[tid][j].val == -1){
			hands[tid][j] = pushCard;
			return;
		}
		else{
			printf("\nERROR! Attempt to push card to full hand.");
		}
	}
}

void waitForTable(intptr_t tid, int bar){
	
	pthread_mutex_lock(&tableMut);
	table++;
	if (table != PLAYERS + 1){
		//printf("\nPlayer %ld waits. Barrier %d.", tid, bar);
		pthread_cond_wait(&tableCond, &tableMut);
		//printf("\nPlayer %ld engages. Barrier %d.", tid, bar);
	}
	else{
		//printf("\nPlayer %ld last.  Barrier %d.", tid, bar);
		pthread_cond_broadcast(&tableCond);
		table = 0;
	}
	pthread_mutex_unlock(&tableMut);
}

void endRound(intptr_t tid){
	pthread_mutex_lock(&tableMut);
	table++;
	if (table != PLAYERS + 1){
		//printf("\nPlayer %ld waits to end round.", tid);
		pthread_cond_wait(&tableCond, &tableMut);
		//printf("\nPlayer %ld ends round.", tid);
	}
	else{
		//printf("\nPlayer %ld last and initiates end round.", tid);
		pthread_cond_broadcast(&tableCond);
		curRound++;
		table = 0;
	}
	pthread_mutex_lock(&fileMut);
	if ((int)tid == PLAYERS){
		fprintf(filePtr, "\nDEALER: end round");
	}
	else{
		fprintf(filePtr, "\nPLAYER %ld: end round", tid+1);
	}
	pthread_mutex_unlock(&fileMut);
	pthread_mutex_unlock(&tableMut);
}

void *dealer(void *id){
	intptr_t tid = (intptr_t)id;
	//printf("\nDealer initiated.");
	genDeck();
	//printDeck();
	logDeck();
	struct card temCard;

	while(curRound < ROUNDS){
		//printf("\n---> Dealer begins round %d!", curRound + 1);
		pthread_mutex_lock(&fileMut);
		fprintf(filePtr, "\nDEALER: begin round %d", curRound + 1);
		pthread_mutex_unlock(&fileMut);
		draw = curRound;
		victor = false;
		shufDeck();
		logDeck();
		//printDeck();
		// SYNCH 1 - Control for start of round.
		waitForTable(tid, 1);
		// ACTION 1 - Dealer deals.
		hc = getHandCount();
		if (hc != 0){
			printf("\nERROR. Began round with non-empty hands.");
			exit(-1);
		}
		// Deal hands.
		for (int k = 0; k < PLAYERS; k++){
			//printf("\nDealer deals to player %d.", k);
			temCard = popDeck();
			//printf("\ntemCard.val %d", temCard.val);
			pushHand(temCard, (intptr_t)k);
		}
		//printf("\nDealer deals.");
		//printDeck();
		// SYNCH 2 - Players wait for dealer to deal.
		waitForTable(tid, 2);
		// ACTION 2 - Players enter draw loop. No action by dealer.
		waitForTable(tid, 3);
		// ACTION 3 - Players return cards to deck.  No action by dealer.
		endRound(tid);
		countDeck();
	}
}

void *player(void *id){
	intptr_t tid = (intptr_t)id;
	//printf("\nPlayer %ld initiated.", tid+1);
	while(curRound < ROUNDS){
		int localRound = curRound;
		//printf("\n---> Player %ld begins round %d!", tid+1, curRound);
		// SYNCH 1 - Control for start of round.
		waitForTable(tid, 1);
		// ACTION 1 - Dealer deals.  No action by players.
		waitForTable(tid, 2);
		//printHand(tid);
		// ACTION 2 - Players enter draw loop.	
		pthread_mutex_lock(&drawMut);
		while (victor == false){
			if (draw == (int)tid){
				logHand(tid);
				// Draw card.
				hc = 0;
				for (int k = 0; k < MAX_HAND; k++){
					if (hands[tid][k].val == -1){
						hands[tid][k] = popDeck();
						//printf("\n> Player %ld draws.", tid+1);
						logDraw(tid, k);
						//printHand(tid);
					}
					else{
						hc++;
						if (hc == MAX_HAND){
							printf("\nERROR! Player %ld cannot draw to full hand!\n", tid+1);
							exit(-1);	
						}
					}
				}
				// Test hand. If victor ...
				if (hands[tid][0].val == hands[tid][1].val){
					// Declare victory.
					victor = true;
					//printf("\n*** ~ Player %ld declares victory! ~ ***", tid+1);
					// Print end of round to screen.
					printf("\n> ROUND %d", curRound);
					printf("\nVictor: Player %ld", tid);
					for (int k = 0; k < PLAYERS; k++){
						printf("\nPlayer %d hand: ", k);
						printHand((intptr_t)k);
					}
					printf("\n");
					// Update logfile.
					pthread_mutex_lock(&fileMut);
					fprintf(filePtr, "\nPLAYER %ld: wins", tid+1);
					pthread_mutex_unlock(&fileMut);
					logHand(tid);
				}
				// If not
				else{
					randCard = rand() % MAX_HAND;  
					//printf("\nrandCard: %d", randCard);
					pushDeck(hands[tid][randCard], tid);
					hands[tid][randCard].val = -1;
					logHand(tid);
					logDeck(tid);
					//printDeck();
				}
				draw = (draw + 1) % PLAYERS;
				pthread_cond_broadcast(&drawCond);
			}
			else{
				pthread_cond_wait(&drawCond, &drawMut);
			}
		}
		//printf("\nPlayer %ld exits draw loop. Round %d.", tid, curRound);
		pthread_mutex_unlock(&drawMut);
		waitForTable(tid, 3);
		// ACTION 3 - Players return cards to deck.
		returnCards(tid);
		// SYNCH 4 - Control for end of round.
		endRound(tid);
	}
}
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_OF_CARDS 52
#define HAND_SIZE 2
#define LOG_FILE "log.txt"
#define NUM_OF_ROUNDS 3
#define NUM_OF_PLAYERS 3

FILE *logFile;
int deck[NUM_OF_CARDS];
int *top, *bottom;
int seed;
int discarded;
int p1_hand [HAND_SIZE];
int p2_hand [HAND_SIZE];
int p3_hand [HAND_SIZE];
pthread_t dealerThread;
pthread_t playerThread1;
pthread_t playerThread2;
pthread_t playerThread3;
bool gameOver = false;
int turn = 0;
int round_num = 1;

pthread_mutex_t player_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t win_condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t dealer_lock = PTHREAD_MUTEX_INITIALIZER;

void setUpGame();
void shuffle();
void takeTurn();
void *player(void *p);
void *dealer(void *d);
void createThreads();
void displayDeck();
void deal();
void displayCards(int *cards, int singleCard, int size);


// run the game
int main(int argc, char *argv[]){

    logFile = fopen(LOG_FILE, "w");

    // set up random
    seed = atoi(argv[1]);
    srand(seed);

    setUpGame();
    shuffle();
    displayDeck();

    // play designated number of rounds
    while(round_num <= NUM_OF_ROUNDS) {
        createThreads();
        round_num++;
        gameOver = false;
    }

    exit(EXIT_SUCCESS);
}


void createThreads(){

    // create
    pthread_create(&dealerThread, NULL, &dealer, NULL); // dealer
    pthread_create(&playerThread1, NULL, &player, (void *)1); // player 1
    pthread_create(&playerThread2, NULL, &player, (void *)2); // player 2
    pthread_create(&playerThread3, NULL, &player, (void *)3); // player 3

    // join
    pthread_join(dealerThread, NULL);
    pthread_join(playerThread1, NULL);
    pthread_join(playerThread2, NULL);
    pthread_join(playerThread3, NULL);
}

void *dealer(void *d){

    shuffle();
    deal();
    printf("\nDEALER: shuffle \n");

    // players take turns going first
    if(round_num == 1) {
        turn = 1;
    } else if(round_num == 2) {
        turn = 2;
    } else {
        turn = 3;
    }

    pthread_cond_broadcast(&condition);

    pthread_mutex_lock(&dealer_lock); // lock
    while(!gameOver){ // while game is not over
        pthread_cond_wait(&win_condition, &dealer_lock);
    }
    pthread_mutex_unlock(&dealer_lock); //unlock

    printf("DEALER: exits round \n");
    pthread_exit(NULL);
}

void *player(void *p) {

    // get player number
    int player = *(int *)&p;

    int hand [HAND_SIZE];
      if(player == 1) {
          hand[0] = p1_hand[0];
          hand[1] = p1_hand[1];
      }
      else if(player == 2) {
          hand[0] = p2_hand[0];
          hand[1] = p2_hand[1];
      }
      else{
        hand[0] = p3_hand[0];
        hand[1] = p3_hand[1];
      }


    while(gameOver == 0){ // while game not over
        pthread_mutex_lock(&player_lock); // lock
        while(gameOver == 0 && !(player == turn || turn == 0)){
           // wait
            pthread_cond_wait(&condition, &player_lock);
        }
        if(gameOver == 0){ // go
            takeTurn(player, &hand);
        }
        pthread_mutex_unlock(&player_lock); // unlock
    }

    printf("PLAYER %d: exits round \n", player);
    pthread_exit(NULL);
}



void takeTurn(int player, int* hand) {

    // print hand before draw
    printf("HAND ");
    displayCards(hand, 0, 1);
    printf("PLAYER %d: hand ", player);
    displayCards(hand, 0, 1);

    // take a card
    hand[1] = *top;
    top++;
    printf("PLAYER %d: draws ", player);
    displayCards(0, hand[1], 1);

    // print hand after draw
//    printf("HAND %d %d\n", hand[0], hand[1]);
//    printf("PLAYER %d: hand %d %d\n", player, hand[0], hand[1]);
    printf("HAND ");
    displayCards(hand, 0, 2);
    printf("PLAYER %d: hand ", player);
    displayCards(hand, 0, 2);

    // check if match
    if (hand[0] == hand[1]) { //match
        printf("WIN yes\n");
        printf("PLAYER %d: wins\n", player);
        gameOver = 1; // game over
        pthread_cond_signal(&win_condition);

    } else { //no match
        printf("WIN no \n");
        top--;
        int *ptr = top;

        while (ptr != bottom) {
            *ptr = *(ptr+1);
            ptr++;
        }

        //discard one card
        int i = rand() % 2; // 0 or 1
        //printf("PLAYER %d: discards %d \n", player, hand[i]);
        printf("PLAYER %d: discards ", player);
        displayCards(0, hand[i], 1);
        *bottom = hand[i];
        if(i == 0){
          hand[0] = hand[1];
        }
        //discarded = hand[i];
        //displayDeck();
        printf("DECK ");
        displayCards(deck, 0, 52);
  }
    turn++;
    if (turn > NUM_OF_PLAYERS){
      turn = 1;
    }
    pthread_cond_broadcast(&condition);
}


void setUpGame(){

  int cardNum = 1;
  int deckNum = 0;
  int suitNum = 4;
  int tempCards = NUM_OF_CARDS;

  // create all the cards
    while( (deckNum < NUM_OF_CARDS) &&
           (cardNum <= (tempCards/suitNum)) ){
       for( int i = 0; i < suitNum; i++ ){
          deck[deckNum] = cardNum;

          deckNum++;
       }
       cardNum++;
    }
    top = deck;
    bottom = deck + (tempCards - 1) ;
}

void shuffle(){

  int tempCards = NUM_OF_CARDS;

  // shuffle the deck
  for (int x = 0; x < (tempCards - 1); x++){
    int temp = deck[x];
    int randNum = rand() % (tempCards - x) + x;
    deck[x] = deck[randNum];
    deck[randNum] = temp;
  }
}

void deal(){

  // deal the deck
    p1_hand[0] = *top;
    top++;
    p2_hand[0] = *top;
    top++;
    p3_hand[0] = *top;
    top++;
}
void displayDeck(){

  //fputs("DECK ", logFile);
  printf("DECK ");
  int *deckPtr = top;
  char str[10] = {0};

  //get integer into char to print to file
  while(deckPtr != bottom){
      //fputs("%d", *deckPtr, logFile);
      printf("%d ", *deckPtr);
      deckPtr++;

      if(deckPtr == bottom){
          //fputs("%d \n", *deckPtr, logFile);
          printf("%d \n", *deckPtr);
      }
  }

  printf("\n");
}

void displayCards(int *cards, int singleCard, int size) {

    int card;
    for(int i=0; i < size; i++) {
        if(singleCard > 0) { // if the card does not exist in an array
            card = singleCard;
        } else {
            card = cards[i];
        }
        if(card == discarded) {
            break;
        } else if(card == 11) {
            printf("J ");
        } else if(card == 12) {
            printf("Q ");
        } else if(card == 13) {
            printf("K ");
        } else if(card == 1) {
            printf("A ");
        } else {
            printf("%d ", card);
        }
    }
    printf("\n");
}
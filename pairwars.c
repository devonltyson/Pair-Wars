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
void deal();
void displayCards(int *cards, int singleCard, int size);


// run the game
int main(int argc, char *argv[]){

    logFile = fopen(LOG_FILE, "w");

    // set up random
    seed = atoi(argv[1]);
    srand(seed);

    // create the deck
    setUpGame();

    // show the deck
    printf("DECK ");
    displayCards(deck, 0, 52);

    // play designated number of rounds
    while(round_num <= NUM_OF_ROUNDS) {
        createThreads(); // run the game using posix threads
        round_num++; // once game is over, increase the round number
        gameOver = false; // reset for the next round
    }

    exit(EXIT_SUCCESS);
}

// create threads for dealer and players
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

// one thread for the dealer
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
        pthread_cond_wait(&win_condition, &dealer_lock); // wait for a win
    }
    pthread_mutex_unlock(&dealer_lock); //unlock when winner determined

    // exit when round is over
    printf("DEALER: exits round \n");
    pthread_exit(NULL);
}

// player thread for each player
void *player(void *p) {

    // get player number
    int player = *(int *)&p;

    // use current players hand
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

    // play until the round is over
    while(gameOver == 0){ // while game not over
        pthread_mutex_lock(&player_lock); // lock
        while(gameOver == 0 && !(player == turn || turn == 0)){ // while game not over and its not their turn
            pthread_cond_wait(&condition, &player_lock); // wait their turn
        }
        if(gameOver == 0){ // player takes their turn
            takeTurn(player, &hand);
        }
        pthread_mutex_unlock(&player_lock); // unlock after turn complete
    }

    // exit when round is over
    printf("PLAYER %d: exits round \n", player);
    pthread_exit(NULL);
}

// each player takes their own turn
void takeTurn(int player, int* hand) {

    // print hand before draw
    printf("HAND ");
    displayCards(hand, 0, 1);
    printf("PLAYER %d: hand ", player);
    displayCards(hand, 0, 1);

    // take a card from the top of the deck
    hand[1] = *top;
    top++;
    printf("PLAYER %d: draws ", player);
    displayCards(0, hand[1], 1);

    // print hand after draw
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

        // shift the deck
        while (ptr != bottom) {
            *ptr = *(ptr+1);
            ptr++;
        }

        //discard one card
        int i = rand() % 2; // get a 0 or 1
        printf("PLAYER %d: discards ", player);
        displayCards(0, hand[i], 1); // use random 0 or 1 as index
        *bottom = hand[i];
        if(i == 0){ // move card to first position in the array
          hand[0] = hand[1];
        }
        printf("DECK ");
        displayCards(deck, 0, 52);
  }
    // signal next players turn
    turn++;
    if (turn > NUM_OF_PLAYERS){
      turn = 1; // go back to player 1 if player 3 just went
    }
    pthread_cond_broadcast(&condition);
}

// make the deck of cards
void setUpGame(){

  int cardNum = 1; // starting card number
  int deckNum = 0; // starting position in the deck
  int suitNum = 4; // number of suits per card number
  int tempCards = NUM_OF_CARDS;

  // create all the cards
    while( (deckNum < NUM_OF_CARDS) &&
           (cardNum <= (tempCards/suitNum)) ){
       for( int i = 0; i < suitNum; i++ ){ // make 4 of each number
          deck[deckNum] = cardNum;
          deckNum++;
       }
       cardNum++;
    }
    top = deck;
    bottom = deck + (tempCards - 1);
    shuffle();
}

// shuffle the entire deck of cards
void shuffle(){

  int tempCards = NUM_OF_CARDS;

  // shuffle the deck
  for (int x = 0; x < (tempCards - 1); x++){
    int temp = deck[x]; // save card at current index
    int randNum = rand() % (tempCards - x) + x; // get a random position
    // switch random card index with current card index
    deck[x] = deck[randNum];
    deck[randNum] = temp;
  }
}

// deal the top card to the players
void deal(){

    // deal the deck by taking from the first element in the array
    p1_hand[0] = *top;
    top++;
    p2_hand[0] = *top;
    top++;
    p3_hand[0] = *top;
    top++;
}

// use an array of cards or a single card
// specify the number of cards with the size variable
void displayCards(int *cards, int singleCard, int size) {

    int card;
    for(int i=0; i < size; i++) {
        if(singleCard > 0) { // if the card does not exist in an array
            card = singleCard; // size should be 1
        } else {
            card = cards[i]; // size can be 1 or more
        }
        if(card == 11) {
            printf("J "); // jack
        } else if(card == 12) {
            printf("Q "); // queen
        } else if(card == 13) {
            printf("K "); // king
        } else if(card == 1) {
            printf("A "); // ace
        } else {
            printf("%d ", card); // any other card
        }
    }
    printf("\n");
}
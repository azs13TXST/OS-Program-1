#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <deque>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>

#define THREADS 3
#define ROUNDS 3
#define SHUFFLE 4
#define LOSER 1
#define WINNER 0

struct Player
{
  int identification;
  std::deque<int> hand;
};

struct Deck
{
  std::deque<int> cards;
};

pthread_mutex_t mutexdeck;
pthread_mutex_t mutexstatus;
pthread_cond_t status_cv;
Deck* deck = new Deck;
int status = LOSER;

void setDeck(Deck*&);
void shuffleDeck(Deck*&, const int = 1);
void dealRound(Player* player[], const int);
void printStatus(const Player* const player[], const int);
void printDeck(const Deck*, const int print = 0);
void printHand(const Player* p, const int print = 0);
void drawCards(Player*& p, Deck*& d);
void cardDiscard(Player*&, Deck*&);
void placeInDeck(Deck*&, const int);
int takeFromDeck();
void* playerTurn(void*);
void concludeRound(Player*& p);
void WriteInLog(const std::string);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Error running " << argv[0] << " <seed_rng> " << std::endl;
    return 1;
  }
  srand(atoi(argv[1]));

  Player* p1 = new Player;
  Player* p2 = new Player;
  Player* p3 = new Player;
  Player* player[THREADS] = {p1,p2,p3};

  for (int i = 0; i < THREADS; i++) {
    player[i]->identification = i;
  }

  pthread_t threads[THREADS];
  pthread_mutex_init(&mutexdeck, NULL);
  pthread_mutex_init(&mutexstatus, NULL);

  for (int r = 0; r < ROUNDS; r++) {
    setDeck(deck);
    std::cout << "ROUND " << r << std::endl << std::endl;

    dealRound(player, THREADS);
    printStatus(player, THREADS);
    int s;
    while (status) {
      for (int i = 0; i < THREADS; i++) {
        s = pthread_create(&threads[i], NULL, playerTurn, (void *)player[i]);
        if (s) {
          printf("ERROR; return code from pthread_create() is %d\n", s);
          exit(-1);
        }
      }

      for (int j = 0; j < THREADS; j++) {
        if (pthread_join(threads[j], NULL)) {
          fprintf(stderr, "Error with thread\n");
          exit(2);
        }
      }
    }
    printStatus(player, THREADS);

    for (int e = 0; e < THREADS; e++) {
      std::string s = "PLAYER ";
      s += std::to_string(player[e]->identification);
      s += ": exits round";
      concludeRound(player[e]);
      WriteInLog(s);
    }
    status = LOSER;
  }
  pthread_mutex_destroy(&mutexdeck);
  pthread_mutex_destroy(&mutexstatus);
  pthread_exit(NULL);
}

void setDeck(Deck*& d) {
  delete d;
  d = new Deck;
  for (int i = 1; i <= 13; i++) {
    for (int j = 0; j < 4; j++) {
      d->cards.push_back(i);
    }
  }
  shuffleDeck(d,SHUFFLE);
}

void shuffleDeck(Deck*& d, const int time) {
  int size = d->cards.size();
  int pos;
  WriteInLog("DEALER: shuffling");
  for (int j = 0; j < time; j++) {
    for (int i = 0 ; i < size; i++) {
      pos = rand() % size;
#ifdef DEBUG_DECK
      cout << "Swapping positions " << i << " & " << position << endl;
#endif
      iter_swap(d->cards.begin() + i, d->cards.begin() + pos);
    }
  }
}


void dealRound(Player* player[], const int player_count) {
  WriteInLog("DEALER: Dealing cards");
  for (int dr = 0; dr < player_count; dr++) {
    player[dr]->hand.push_back(takeFromDeck());
  }
}


void printStatus(const Player* const player[], const int player_count) { 
  for (int ps = 0; ps < player_count; ps++) {
    printHand(player[ps], 1);
  }
  printDeck(deck, 1);
  std::cout << std::endl;
}

void printDeck(const Deck* d, const int print) {
  std::string pd = "The number of cards in the deck is ";
  pd += std::to_string(d->cards.size());
  pd += ". Current deck:\n";

  for (int i = d->cards.size() - 1; i >= 0; i--) {
    pd += std::to_string(d->cards.at(i));
    pd+= " ";
  }
  WriteInLog(pd);

  if(print) {
    std::cout << pd << std::endl;
  }
}

void printHand(const Player* p, const int print) {
  std::string ph = "PLAYER "; 
  ph += std::to_string(p->identification); 
  ph+=  ": HAND ";

  for (int i = p->hand.size() - 1; i >= 0; i--) {
    ph +=  std::to_string(p->hand.at(i)); ph += " ";
  }
  WriteInLog(ph);
  if(print) {
    std::cout << ph << std::endl;
  }
}

void drawCards(Player*& p, Deck*& d) {
  std::string dc = "";
  dc += "PLAYER "; dc += std::to_string(p->identification); dc+= ": draws ";
  if (d->cards.size() > 0) {
    int card = takeFromDeck();
    dc += std::to_string(card);
    p->hand.push_back(card);
  } 
  else {
    dc += "nothing from empty deck." ;
  }
  WriteInLog(dc);
}

void cardDiscard(Player*& p, Deck*& d) {
  std::string cd = "";
  cd += "PLAYER "; cd += std::to_string(p->identification); cd += ": discards ";

  int cards_in_hand = p->hand.size();
  int position = rand() % cards_in_hand;

  cd += std::to_string(p->hand.at(position));

  placeInDeck(d, p->hand.at(position));
  p->hand.erase(p->hand.begin() + position);

  WriteInLog(cd);
}

void placeInDeck(Deck*& d, const int card) {
  d->cards.push_back(card);
}

int takeFromDeck() {
  int card =  deck->cards.front();
  deck->cards.pop_front();
  return card;
}

void* playerTurn(void* player) {
  Player* p = (Player*)player;

  pthread_mutex_lock(&mutexdeck);
  if (p->hand.size() >= 2) {
    cardDiscard(p, deck);
    printHand(p);
  }
#ifdef FAKEOUT_DELAY
  usleep(rand() % 1000000);
#endif
  drawCards(p, deck);
  printHand(p);
  pthread_mutex_lock (&mutexstatus);
  if (p->hand.at(0) ==  p->hand.at(1)) {
    pthread_cond_signal(&status_cv);
    status = WINNER;
    std::string l = "PLAYER ";
    l += std::to_string(p->identification);
    l += " WINS!";
    WriteInLog(l);
  }
  pthread_mutex_unlock (&mutexstatus);
  pthread_mutex_unlock (&mutexdeck);

  pthread_exit(NULL);
}

void concludeRound(Player*& p) {
  int inHand = p->hand.size();
  for (int i = inHand - 1; i >= 0; i--) {
    p->hand.erase(p->hand.begin() + i);
  }
}

void WriteInLog(std::string l) {
#ifdef STATE_EVENTS
  cout << l << endl;
#else
  std::ofstream fout;
  std::ifstream fin;
  fin.open("log.txt");
  fout.open ("log.txt",std::ios::app);
  if (fin.is_open()) {
    fout << l << std::endl;
  }
  fin.close();
  fout.close();
#endif
}

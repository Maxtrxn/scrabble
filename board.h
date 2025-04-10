#ifndef BOARD_H
#define BOARD_H

#include "scrabble.h"

// Déclaration du plateau bonus (défini dans board.c)
extern int bonusBoard[15][15];

// Fonctions pour la gestion des lettres et du plateau
int getLetterScore(char letter);
char drawRandomLetter(void);
bool canPlaceWord(const char *word, int startX, int startY, char dir,
                  char **board, int boardSize, const char *rack, int totalPoints);
void placeWord(const char *word, int startX, int startY, char dir,
               char **board, char *rack);
int recalcTotalScore(char **board, int boardSize);
bool validatePlacement(const char *word, int startX, int startY, char dir,
                       char **board, int boardSize, DictionaryEntry *dictionary);
void findBestMove(char **board, int boardSize, DictionaryEntry *dictionary,
                  char *rack, int *totalPoints, int bonusBoard[15][15]);

#endif  // BOARD_H

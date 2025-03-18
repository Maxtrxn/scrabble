#ifndef SCRABBLE_H
#define SCRABBLE_H

// Définition de la version POSIX
#define _POSIX_C_SOURCE 200809L

// Inclusion des bibliothèques SDL et TTF
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// Bibliothèques standards
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

// UT_hash (pour le dictionnaire)
#include "uthash.h"

// Définition des constantes
#define WINDOW_WIDTH      800
#define WINDOW_HEIGHT     900

#define BOARD_HEIGHT      800                        // Hauteur de la zone du plateau
#define SCRABBLE_RACK_HEIGHT       50
#define INPUT_AREA_HEIGHT (WINDOW_HEIGHT - BOARD_HEIGHT - SCRABBLE_RACK_HEIGHT)
#define BOARD_MARGIN      50

// Énumération pour l'état de saisie
typedef enum {
    STATE_IDLE,
    STATE_INPUT_TEXT,
    STATE_INPUT_DIRECTION
} InputState;

// Structure pour le dictionnaire (UT_hash)
typedef struct {
    char word[100];  // La taille peut être adaptée
    UT_hash_handle hh;
} DictionaryEntry;

// Prototypes de fonctions globales
// (Vous pouvez les regrouper par module dans leurs fichiers respectifs, mais les déclarer ici
//  permet d’avoir un point de référence commun pour les autres modules.)
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

// Prototypes pour le rendu graphique
void drawGrid(SDL_Renderer *renderer, int boardSize, int boardDrawWidth, int boardDrawHeight);
void drawBoard(SDL_Renderer *renderer, TTF_Font *boardFont, TTF_Font *valueFont,
               char **board, int boardSize, int boardDrawWidth, int boardDrawHeight, int gridThickness);
void drawRack(SDL_Renderer *renderer, TTF_Font *rackFont, TTF_Font *valueFont,
              char *rack, int rackAreaWidth, int startXRack, int buttonMargin, int buttonWidth, int buttonHeight,
              TTF_Font *inputFont);
void drawInputArea(SDL_Renderer *renderer, TTF_Font *inputFont, InputState currentState, char *inputBuffer, int totalPoints);

#endif  // SCRABBLE_H

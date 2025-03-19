#ifndef UTILS_H
#define UTILS_H

#include "scrabble.h"
#include "dictionary.h"

// Structure regroupant les ressources SDL et TTF
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *boardFont;
    TTF_Font *rackFont;
    TTF_Font *inputFont;
    TTF_Font *valueFont;
} Resources;

// Prototypes
int initResources(Resources *res);
char **initBoard(int boardSize);
void freeBoard(char **board, int boardSize);
void cleanup(Resources *res, DictionaryEntry *dictionaryHash, char **board, int boardSize);

#endif // UTILS_H

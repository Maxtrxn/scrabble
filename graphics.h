#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "scrabble.h"

// Définition des couleurs utilisées pour l'affichage
extern SDL_Color BACKGROUND_COLOR;
extern SDL_Color GRID_COLOR;
extern SDL_Color TEXT_COLOR;
extern SDL_Color INPUT_BG_COLOR;


// Fonctions d'affichage SDL
void drawGrid(SDL_Renderer *renderer, int boardSize, int boardDrawWidth, int boardDrawHeight);
void drawBoard(SDL_Renderer *renderer, TTF_Font *boardFont, TTF_Font *valueFont,
               char **board, int boardSize, int boardDrawWidth, int boardDrawHeight, int gridThickness);
void drawRack(SDL_Renderer *renderer, TTF_Font *rackFont, TTF_Font *valueFont,
              char *rack, int rackAreaWidth, int startXRack, int buttonMargin,
              int buttonWidth, int buttonHeight, TTF_Font *inputFont);
void drawInputArea(SDL_Renderer *renderer, TTF_Font *inputFont, InputState currentState, char *inputBuffer, int totalPoints);

#endif  // GRAPHICS_H

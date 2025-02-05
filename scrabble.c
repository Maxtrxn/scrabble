#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define WINDOW_WIDTH      800
#define WINDOW_HEIGHT     900
#define BOARD_HEIGHT      800
#define INPUT_AREA_HEIGHT (WINDOW_HEIGHT - BOARD_HEIGHT)

// Marge autour de la zone de dessin du plateau
#define BOARD_MARGIN      50

// Couleurs générales
static SDL_Color BACKGROUND_COLOR = {255, 255, 255, 255};  // Fond général (blanc)
static SDL_Color GRID_COLOR       = {255, 255, 255, 255};  // Bordure de la grille (blanc)
static SDL_Color TEXT_COLOR       = {0, 0, 0, 255};        // Texte (noir)
static SDL_Color INPUT_BG_COLOR   = {200, 200, 200, 255};  // Zone d'entrée (gris clair)

// États de saisie
typedef enum {
    STATE_IDLE,           // Aucun input en cours
    STATE_INPUT_TEXT,     // Saisie d'une lettre ou d'un mot
    STATE_INPUT_DIRECTION // Saisie de la direction (h/v) pour un mot
} InputState;

// Retourne le score d'une lettre (en majuscule)
int getLetterScore(char letter) {
    letter = toupper(letter);
    if (strchr("AEILNORSTU", letter)) return 1;
    else if (strchr("DGM", letter)) return 2;
    else if (strchr("BCP", letter)) return 3;
    else if (strchr("FHV", letter)) return 4;
    else if (strchr("JQ", letter)) return 8;
    else if (strchr("KWXYZ", letter)) return 10;
    return 0;
}

int main(int argc, char* argv[]) {
    // Taille fixe du plateau (15x15)
    int boardSize = 15;

    // Allocation du plateau (tableau 2D de caractères)
    char **board = malloc(boardSize * sizeof(char *));
    if (!board) {
        fprintf(stderr, "Erreur d'allocation mémoire pour le plateau.\n");
        return EXIT_FAILURE;
    }
    for (int i = 0; i < boardSize; i++) {
        board[i] = malloc(boardSize * sizeof(char));
        if (!board[i]) {
            fprintf(stderr, "Erreur d'allocation mémoire.\n");
            return EXIT_FAILURE;
        }
        memset(board[i], ' ', boardSize);
    }

    // Définition du plateau des bonus (standard Scrabble)
    // Codes : 0 = normal, 1 = Triple mot, 2 = Double mot, 3 = Triple lettre, 4 = Double lettre
    int bonusBoard[15][15] = {
        {1, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 1},
        {0, 2, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 2, 0},
        {0, 0, 2, 0, 0, 0, 4, 0, 4, 0, 0, 0, 2, 0, 0},
        {4, 0, 0, 2, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 4},
        {0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0},
        {0, 3, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 3, 0},
        {0, 0, 4, 0, 0, 0, 4, 0, 4, 0, 0, 0, 4, 0, 0},
        {1, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 4, 0, 0, 1},
        {0, 0, 4, 0, 0, 0, 4, 0, 4, 0, 0, 0, 4, 0, 0},
        {0, 3, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 3, 0},
        {0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0},
        {4, 0, 0, 2, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 4},
        {0, 0, 2, 0, 0, 0, 4, 0, 4, 0, 0, 0, 2, 0, 0},
        {0, 2, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 2, 0},
        {1, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 1}
    };

    // Initialisation de SDL et SDL_ttf
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Erreur SDL_Init: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "Erreur TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Window *window = SDL_CreateWindow("Scrabble Simplifié",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Erreur SDL_CreateWindow: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Erreur SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    // Chargement des polices :
    // - boardFont pour les lettres sur la grille et le compteur (taille réduite à 28)
    // - inputFont pour la zone de saisie (taille 24)
    // - valueFont pour afficher la valeur de la lettre (taille réduite à 12)
    TTF_Font *boardFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 28);
    if (!boardFont) {
        fprintf(stderr, "Erreur TTF_OpenFont (boardFont): %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
    TTF_Font *inputFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
    if (!inputFont) {
        fprintf(stderr, "Erreur TTF_OpenFont (inputFont): %s\n", TTF_GetError());
        TTF_CloseFont(boardFont);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
    TTF_Font *valueFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12);
    if (!valueFont) {
        fprintf(stderr, "Erreur TTF_OpenFont (valueFont): %s\n", TTF_GetError());
        TTF_CloseFont(inputFont);
        TTF_CloseFont(boardFont);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
    
    // Variables de gestion de la saisie
    InputState currentState = STATE_IDLE;
    char inputBuffer[50] = "";
    int inputLength = 0;
    char tempWord[50] = "";
    int selectedCellX = -1, selectedCellY = -1;
    int lastWordScore = 0;
    
    bool quit = false;
    SDL_Event e;
    
    // Calcul des dimensions de la zone de dessin du plateau
    int boardDrawWidth  = WINDOW_WIDTH - 2 * BOARD_MARGIN;
    int boardDrawHeight = BOARD_HEIGHT - 2 * BOARD_MARGIN;
    float cellWidth  = (float)boardDrawWidth / boardSize;
    float cellHeight = (float)boardDrawHeight / boardSize;
    
    int gridThickness = 2;  // Épaisseur de la grille (en pixels)
    
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (currentState == STATE_IDLE) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int mouseX = e.button.x;
                    int mouseY = e.button.y;
                    if (mouseX >= BOARD_MARGIN && mouseX < (WINDOW_WIDTH - BOARD_MARGIN) &&
                        mouseY >= BOARD_MARGIN && mouseY < (BOARD_MARGIN + boardDrawHeight)) {
                        selectedCellX = (mouseX - BOARD_MARGIN) / cellWidth;
                        selectedCellY = (mouseY - BOARD_MARGIN) / cellHeight;
                        currentState = STATE_INPUT_TEXT;
                        inputBuffer[0] = '\0';
                        inputLength = 0;
                        SDL_StartTextInput();
                    }
                }
            }
            else if (currentState == STATE_INPUT_TEXT) {
                if (e.type == SDL_TEXTINPUT) {
                    if (inputLength + strlen(e.text.text) < sizeof(inputBuffer) - 1) {
                        strcat(inputBuffer, e.text.text);
                        inputLength = strlen(inputBuffer);
                        for (int i = 0; i < inputLength; i++) {
                            inputBuffer[i] = toupper(inputBuffer[i]);
                        }
                    }
                }
                else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_BACKSPACE && inputLength > 0) {
                        inputBuffer[inputLength - 1] = '\0';
                        inputLength--;
                    }
                    else if (e.key.keysym.sym == SDLK_RETURN) {
                        SDL_StopTextInput();
                        if (inputLength == 0) {
                            currentState = STATE_IDLE;
                        }
                        else if (inputLength == 1) {
                            int letterScore = getLetterScore(inputBuffer[0]);
                            int bonus = bonusBoard[selectedCellY][selectedCellX];
                            if (selectedCellX == 7 && selectedCellY == 7) bonus = 2;
                            if (bonus == 3) letterScore *= 3;
                            else if (bonus == 4) letterScore *= 2;
                            else if (bonus == 1) letterScore *= 3;
                            else if (bonus == 2) letterScore *= 2;
                            lastWordScore = letterScore;
                            board[selectedCellY][selectedCellX] = inputBuffer[0];
                            currentState = STATE_IDLE;
                        }
                        else {
                            strcpy(tempWord, inputBuffer);
                            inputBuffer[0] = '\0';
                            inputLength = 0;
                            currentState = STATE_INPUT_DIRECTION;
                        }
                    }
                    else if (e.key.keysym.sym == SDLK_ESCAPE) {
                        SDL_StopTextInput();
                        currentState = STATE_IDLE;
                    }
                }
            }
            else if (currentState == STATE_INPUT_DIRECTION) {
                if (e.type == SDL_KEYDOWN) {
                    char dir = tolower((char)e.key.keysym.sym);
                    if (dir == 'h' || dir == 'v') {
                        int len = strlen(tempWord);
                        bool valid = true;
                        int computedScore = 0;
                        int wordMultiplier = 1;
                        if (dir == 'h') {
                            if (selectedCellX + len > boardSize)
                                valid = false;
                            for (int i = 0; i < len && valid; i++) {
                                if (board[selectedCellY][selectedCellX + i] != ' ' &&
                                    board[selectedCellY][selectedCellX + i] != tempWord[i])
                                    valid = false;
                            }
                            if (valid) {
                                for (int i = 0; i < len; i++) {
                                    int x = selectedCellX + i;
                                    int y = selectedCellY;
                                    board[y][x] = tempWord[i];
                                    int letterScore = getLetterScore(tempWord[i]);
                                    int bonus = bonusBoard[y][x];
                                    if (x == 7 && y == 7) bonus = 2;
                                    if (bonus == 3) letterScore *= 3;
                                    else if (bonus == 4) letterScore *= 2;
                                    else if (bonus == 1) wordMultiplier *= 3;
                                    else if (bonus == 2) wordMultiplier *= 2;
                                    computedScore += letterScore;
                                }
                                computedScore *= wordMultiplier;
                                lastWordScore = computedScore;
                            }
                        }
                        else if (dir == 'v') {
                            if (selectedCellY + len > boardSize)
                                valid = false;
                            for (int i = 0; i < len && valid; i++) {
                                if (board[selectedCellY + i][selectedCellX] != ' ' &&
                                    board[selectedCellY + i][selectedCellX] != tempWord[i])
                                    valid = false;
                            }
                            if (valid) {
                                for (int i = 0; i < len; i++) {
                                    int x = selectedCellX;
                                    int y = selectedCellY + i;
                                    board[y][x] = tempWord[i];
                                    int letterScore = getLetterScore(tempWord[i]);
                                    int bonus = bonusBoard[y][x];
                                    if (x == 7 && y == 7) bonus = 2;
                                    if (bonus == 3) letterScore *= 3;
                                    else if (bonus == 4) letterScore *= 2;
                                    else if (bonus == 1) wordMultiplier *= 3;
                                    else if (bonus == 2) wordMultiplier *= 2;
                                    computedScore += letterScore;
                                }
                                computedScore *= wordMultiplier;
                                lastWordScore = computedScore;
                            }
                        }
                        currentState = STATE_IDLE;
                    }
                    else if (e.key.keysym.sym == SDLK_ESCAPE) {
                        currentState = STATE_IDLE;
                    }
                }
            }
        } // Fin de la gestion des événements
        
        // Rendu graphique
        SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, BACKGROUND_COLOR.a);
        SDL_RenderClear(renderer);
        
        // 1. Dessin du plateau et des bonus
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                SDL_Rect cellRect = {
                    BOARD_MARGIN + (int)(x * cellWidth),
                    BOARD_MARGIN + (int)(y * cellHeight),
                    (int)cellWidth,
                    (int)cellHeight
                };
                // Cas particulier pour la case centrale : affichée en doré.
                if (x == 7 && y == 7) {
                    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255); // Doré
                }
                else if (bonusBoard[y][x] == 1) {
                    // Triple mot → Rouge
                    SDL_SetRenderDrawColor(renderer, 200, 39, 34, 255);
                }
                else if (bonusBoard[y][x] == 2) {
                    // Double mot → Orange
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
                }
                else if (bonusBoard[y][x] == 3) {
                    // Triple lettre → Bleu
                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                }
                else if (bonusBoard[y][x] == 4) {
                    // Double lettre → Bleu clair
                    SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255);
                }
                else {
                    // Case normale → Vert neutre
                    SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
                }
                SDL_RenderFillRect(renderer, &cellRect);
            }
        }
        
        // 1.5. Dessin du petit carré beige sur les cases où une lettre a été placée
        // Le carré occupe 80 % de la taille de la case et est centré (calcul avec round pour plus de précision).
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                if (board[y][x] != ' ') {
                    int overlayWidth = (int)round(cellWidth * 0.8);
                    int overlayHeight = (int)round(cellHeight * 0.8);
                    int offsetX = (int)round((cellWidth - overlayWidth) / 2.0);
                    int offsetY = (int)round((cellHeight - overlayHeight) / 2.0);
                    SDL_Rect overlayRect = {
                        BOARD_MARGIN + (int)(x * cellWidth) + offsetX,
                        BOARD_MARGIN + (int)(y * cellHeight) + offsetY,
                        overlayWidth,
                        overlayHeight
                    };
                    SDL_SetRenderDrawColor(renderer, 245, 245, 220, 255); // Beige opaque
                    SDL_RenderFillRect(renderer, &overlayRect);
                }
            }
        }
        
        // 2. Dessin de la grille avec une épaisseur augmentée
        SDL_SetRenderDrawColor(renderer, GRID_COLOR.r, GRID_COLOR.g, GRID_COLOR.b, GRID_COLOR.a);
        // Lignes verticales
        for (int i = 0; i <= boardSize; i++) {
            int x = BOARD_MARGIN + (int)(i * cellWidth);
            for (int offset = 0; offset < gridThickness; offset++) {
                SDL_RenderDrawLine(renderer, x + offset, BOARD_MARGIN, x + offset, BOARD_MARGIN + boardDrawHeight);
            }
        }
        // Lignes horizontales
        for (int j = 0; j <= boardSize; j++) {
            int y = BOARD_MARGIN + (int)(j * cellHeight);
            for (int offset = 0; offset < gridThickness; offset++) {
                SDL_RenderDrawLine(renderer, BOARD_MARGIN, y + offset, BOARD_MARGIN + boardDrawWidth, y + offset);
            }
        }
        
        // 3. Dessin des lettres déjà placées sur la grille et de leur valeur (coin inférieur droit)
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                char letter = board[y][x];
                if (letter != ' ') {
                    // Affichage de la lettre principale (centrée)
                    char text[2] = { letter, '\0' };
                    SDL_Surface *textSurface = TTF_RenderText_Blended(boardFont, text, TEXT_COLOR);
                    if (textSurface) {
                        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                        int textW, textH;
                        SDL_QueryTexture(textTexture, NULL, NULL, &textW, &textH);
                        int posX = BOARD_MARGIN + (int)(x * cellWidth + (cellWidth - textW) / 2);
                        int posY = BOARD_MARGIN + (int)(y * cellHeight + (cellHeight - textH) / 2);
                        SDL_Rect dstRect = { posX, posY, textW, textH };
                        SDL_RenderCopy(renderer, textTexture, NULL, &dstRect);
                        SDL_DestroyTexture(textTexture);
                        SDL_FreeSurface(textSurface);
                    }
                    // Affichage de la valeur de la lettre dans le coin inférieur droit (avec valueFont, taille réduite)
                    char valueText[4];
                    sprintf(valueText, "%d", getLetterScore(letter));
                    SDL_Surface *valueSurface = TTF_RenderText_Blended(valueFont, valueText, TEXT_COLOR);
                    if (valueSurface) {
                        SDL_Texture *valueTexture = SDL_CreateTextureFromSurface(renderer, valueSurface);
                        int valueW, valueH;
                        SDL_QueryTexture(valueTexture, NULL, NULL, &valueW, &valueH);
                        int valuePosX = BOARD_MARGIN + (int)(x * cellWidth) + (int)cellWidth - valueW - 4; // marge de 2 pixels
                        int valuePosY = BOARD_MARGIN + (int)(y * cellHeight) + (int)cellHeight - valueH - 4;
                        SDL_Rect valueRect = { valuePosX, valuePosY, valueW, valueH };
                        SDL_RenderCopy(renderer, valueTexture, NULL, &valueRect);
                        SDL_DestroyTexture(valueTexture);
                        SDL_FreeSurface(valueSurface);
                    }
                }
            }
        }
        
        // 4. Zone d'entrée (en bas de la fenêtre)
        SDL_Rect inputRect = { 0, BOARD_HEIGHT, WINDOW_WIDTH, INPUT_AREA_HEIGHT };
        SDL_SetRenderDrawColor(renderer, INPUT_BG_COLOR.r, INPUT_BG_COLOR.g, INPUT_BG_COLOR.b, INPUT_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &inputRect);
        
        // Texte d'instruction dans la zone d'entrée
        char displayText[100];
        if (currentState == STATE_IDLE) {
            snprintf(displayText, sizeof(displayText), "Cliquez sur une case pour jouer");
        }
        else if (currentState == STATE_INPUT_TEXT) {
            snprintf(displayText, sizeof(displayText), "Entrez une lettre ou un mot: %s", inputBuffer);
        }
        else if (currentState == STATE_INPUT_DIRECTION) {
            snprintf(displayText, sizeof(displayText), "Entrez la direction (h/v): ");
        }
        
        SDL_Surface *promptSurface = TTF_RenderText_Blended(inputFont, displayText, TEXT_COLOR);
        if (promptSurface) {
            SDL_Texture *promptTexture = SDL_CreateTextureFromSurface(renderer, promptSurface);
            int textW, textH;
            SDL_QueryTexture(promptTexture, NULL, NULL, &textW, &textH);
            int posX = 10;
            int posY = BOARD_HEIGHT + (INPUT_AREA_HEIGHT - textH) / 2;
            SDL_Rect dstRect = { posX, posY, textW, textH };
            SDL_RenderCopy(renderer, promptTexture, NULL, &dstRect);
            SDL_DestroyTexture(promptTexture);
            SDL_FreeSurface(promptSurface);
        }
        
        // 5. Affichage du score dans le coin supérieur droit
        char scoreText[50];
        snprintf(scoreText, sizeof(scoreText), "Points: %d", lastWordScore);
        SDL_Surface *scoreSurface = TTF_RenderText_Blended(boardFont, scoreText, TEXT_COLOR);
        if (scoreSurface) {
            SDL_Texture *scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
            int textW, textH;
            SDL_QueryTexture(scoreTexture, NULL, NULL, &textW, &textH);
            int posX = WINDOW_WIDTH - textW - 10;
            int posY = 10;
            SDL_Rect scoreRect = { posX, posY, textW, textH };
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
            SDL_DestroyTexture(scoreTexture);
            SDL_FreeSurface(scoreSurface);
        }
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }
    
    // Libération de la mémoire allouée
    for (int i = 0; i < boardSize; i++) {
        free(board[i]);
    }
    free(board);
    
    TTF_CloseFont(valueFont);
    TTF_CloseFont(boardFont);
    TTF_CloseFont(inputFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}


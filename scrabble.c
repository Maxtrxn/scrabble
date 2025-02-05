#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

// Dimensions de la fenêtre et répartition en deux zones
#define WINDOW_WIDTH       800
#define WINDOW_HEIGHT      900
#define BOARD_HEIGHT       800
#define INPUT_AREA_HEIGHT  (WINDOW_HEIGHT - BOARD_HEIGHT)

// Couleurs
static SDL_Color BACKGROUND_COLOR = {255, 255, 255, 255};   // Blanc
static SDL_Color GRID_COLOR       = {0,   0,   0,   255};   // Noir
static SDL_Color TEXT_COLOR       = {0,   0,   0,   255};   // Noir
static SDL_Color INPUT_BG_COLOR   = {200, 200, 200, 255};   // Gris clair

// États de saisie
typedef enum {
    STATE_IDLE,           // Rien en cours
    STATE_INPUT_TEXT,     // Saisie de lettre/mot
    STATE_INPUT_DIRECTION // Saisie de la direction (h/v) pour un mot
} InputState;

int main(int argc, char* argv[]) {
    // Pour simplifier, on fixe la taille du plateau (par exemple 15)
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

    // Ouvre la police (modifie le chemin vers une police existante sur ton système)
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
    if (!font) {
        fprintf(stderr, "Erreur TTF_OpenFont: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Variables de gestion de la saisie
    InputState currentState = STATE_IDLE;
    char inputBuffer[50] = "";  // Buffer pour la saisie de texte
    int inputLength = 0;
    char tempWord[50] = "";     // Stocke le mot saisi s'il comporte plusieurs lettres
    int selectedCellX = -1, selectedCellY = -1;  // Case cliquée

    bool quit = false;
    SDL_Event e;

    // Boucle principale
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }

            // État "aucune saisie" : on attend un clic sur une case du plateau
            if (currentState == STATE_IDLE) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int mouseX = e.button.x;
                    int mouseY = e.button.y;
                    // On ne traite que les clics dans la zone du plateau
                    if (mouseY < BOARD_HEIGHT) {
                        selectedCellX = mouseX / (WINDOW_WIDTH / boardSize);
                        selectedCellY = mouseY / (BOARD_HEIGHT / boardSize);
                        currentState = STATE_INPUT_TEXT;
                        inputBuffer[0] = '\0';
                        inputLength = 0;
                        // Active la saisie texte par SDL
                        SDL_StartTextInput();
                    }
                }
            }
            // État "saisie de lettre ou mot"
            else if (currentState == STATE_INPUT_TEXT) {
                if (e.type == SDL_TEXTINPUT) {
                    // Ajoute le texte saisi au buffer (en vérifiant la taille max)
                    if (inputLength + strlen(e.text.text) < sizeof(inputBuffer) - 1) {
                        strcat(inputBuffer, e.text.text);
                        inputLength = strlen(inputBuffer);
                        // Convertit en majuscules
                        for (int i = 0; i < inputLength; i++) {
                            inputBuffer[i] = toupper(inputBuffer[i]);
                        }
                    }
                }
                if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_BACKSPACE && inputLength > 0) {
                        inputBuffer[inputLength - 1] = '\0';
                        inputLength--;
                    }
                    else if (e.key.keysym.sym == SDLK_RETURN) {
                        // Fin de la saisie
                        SDL_StopTextInput();
                        if (inputLength == 0) {
                            // Saisie vide : on annule
                            currentState = STATE_IDLE;
                        }
                        else if (inputLength == 1) {
                            // Saisie d'une seule lettre
                            board[selectedCellY][selectedCellX] = inputBuffer[0];
                            currentState = STATE_IDLE;
                        }
                        else {
                            // Saisie d'un mot
                            strcpy(tempWord, inputBuffer);
                            inputBuffer[0] = '\0';
                            inputLength = 0;
                            currentState = STATE_INPUT_DIRECTION;
                        }
                    }
                    else if (e.key.keysym.sym == SDLK_ESCAPE) {
                        // Annulation de la saisie
                        SDL_StopTextInput();
                        currentState = STATE_IDLE;
                    }
                }
            }
            // État "saisie de la direction" (pour un mot de plusieurs lettres)
            else if (currentState == STATE_INPUT_DIRECTION) {
                if (e.type == SDL_KEYDOWN) {
                    char dir = tolower((char)e.key.keysym.sym);
                    if (dir == 'h' || dir == 'v') {
                        int len = strlen(tempWord);
                        bool valid = true;
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
                                    board[selectedCellY][selectedCellX + i] = tempWord[i];
                                }
                            }
                        }
                        else { // direction verticale
                            if (selectedCellY + len > boardSize)
                                valid = false;
                            for (int i = 0; i < len && valid; i++) {
                                if (board[selectedCellY + i][selectedCellX] != ' ' &&
                                    board[selectedCellY + i][selectedCellX] != tempWord[i])
                                    valid = false;
                            }
                            if (valid) {
                                for (int i = 0; i < len; i++) {
                                    board[selectedCellY + i][selectedCellX] = tempWord[i];
                                }
                            }
                        }
                        // On pourrait afficher un message d'erreur si placement invalide
                        currentState = STATE_IDLE;
                    }
                    else if (e.key.keysym.sym == SDLK_ESCAPE) {
                        currentState = STATE_IDLE;
                    }
                }
            }
        } // Fin de la gestion des événements

        // Rendu graphique

        // Efface l'écran
        SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, BACKGROUND_COLOR.a);
        SDL_RenderClear(renderer);

        // --- 1. Dessin du plateau dans la zone supérieure ---
        float cellWidth  = (float)WINDOW_WIDTH / boardSize;
        float cellHeight = (float)BOARD_HEIGHT / boardSize;

        // Dessine la grille
        SDL_SetRenderDrawColor(renderer, GRID_COLOR.r, GRID_COLOR.g, GRID_COLOR.b, GRID_COLOR.a);
        for (int i = 0; i <= boardSize; i++) {
            int x = (int)(i * cellWidth);
            SDL_RenderDrawLine(renderer, x, 0, x, BOARD_HEIGHT);
        }
        for (int j = 0; j <= boardSize; j++) {
            int y = (int)(j * cellHeight);
            SDL_RenderDrawLine(renderer, 0, y, WINDOW_WIDTH, y);
        }

        // Dessine les lettres déjà placées
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                char letter = board[y][x];
                if (letter != ' ') {
                    char text[2] = { letter, '\0' };
                    SDL_Surface *textSurface = TTF_RenderText_Blended(font, text, TEXT_COLOR);
                    if (textSurface) {
                        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                        int textW, textH;
                        SDL_QueryTexture(textTexture, NULL, NULL, &textW, &textH);
                        int posX = (int)(x * cellWidth + (cellWidth - textW) / 2);
                        int posY = (int)(y * cellHeight + (cellHeight - textH) / 2);
                        SDL_Rect dstRect = { posX, posY, textW, textH };
                        SDL_RenderCopy(renderer, textTexture, NULL, &dstRect);
                        SDL_DestroyTexture(textTexture);
                        SDL_FreeSurface(textSurface);
                    }
                }
            }
        }

        // --- 2. Dessin de la zone d'entrée (zone inférieure) ---
        SDL_Rect inputRect = { 0, BOARD_HEIGHT, WINDOW_WIDTH, INPUT_AREA_HEIGHT };
        SDL_SetRenderDrawColor(renderer, INPUT_BG_COLOR.r, INPUT_BG_COLOR.g, INPUT_BG_COLOR.b, INPUT_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &inputRect);

        // Prépare le texte à afficher dans la zone d'entrée
        const char *prompt;
        char displayText[100];
        if (currentState == STATE_IDLE) {
            prompt = "Cliquez sur une case pour jouer";
            snprintf(displayText, sizeof(displayText), "%s", prompt);
        }
        else if (currentState == STATE_INPUT_TEXT) {
            prompt = "Entrez une lettre ou un mot: ";
            snprintf(displayText, sizeof(displayText), "%s%s", prompt, inputBuffer);
        }
        else if (currentState == STATE_INPUT_DIRECTION) {
            prompt = "Entrez la direction (h/v): ";
            snprintf(displayText, sizeof(displayText), "%s", prompt);
        }

        // Rend le texte avec SDL_ttf
        SDL_Surface *promptSurface = TTF_RenderText_Blended(font, displayText, TEXT_COLOR);
        if (promptSurface) {
            SDL_Texture *promptTexture = SDL_CreateTextureFromSurface(renderer, promptSurface);
            int textW, textH;
            SDL_QueryTexture(promptTexture, NULL, NULL, &textW, &textH);
            int posX = 10; // un peu de marge
            int posY = BOARD_HEIGHT + (INPUT_AREA_HEIGHT - textH) / 2;
            SDL_Rect dstRect = { posX, posY, textW, textH };
            SDL_RenderCopy(renderer, promptTexture, NULL, &dstRect);
            SDL_DestroyTexture(promptTexture);
            SDL_FreeSurface(promptSurface);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // environ 60 FPS
    }

    // Libère la mémoire allouée pour le plateau
    for (int i = 0; i < boardSize; i++) {
        free(board[i]);
    }
    free(board);

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}


#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Dimensions de la fenêtre
#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 800

// Couleurs
static SDL_Color BACKGROUND_COLOR = { 255, 255, 255, 255 };  // Blanc
static SDL_Color GRID_COLOR       = { 0,   0,   0,   255 };  // Noir
static SDL_Color TEXT_COLOR       = { 0,   0,   0,   255 };  // Noir
static SDL_Color INPUT_BG_COLOR   = { 200, 200, 200, 255 };  // Gris clair

// Fonction pour dessiner la grille
void drawGrid(SDL_Renderer *renderer, int boardSize) {
    SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, BACKGROUND_COLOR.a);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, GRID_COLOR.r, GRID_COLOR.g, GRID_COLOR.b, GRID_COLOR.a);

    float cellWidth  = (float)WINDOW_WIDTH  / boardSize;
    float cellHeight = (float)WINDOW_HEIGHT / boardSize;

    for (int i = 0; i <= boardSize; i++) {
        int x = (int)(i * cellWidth);
        SDL_RenderDrawLine(renderer, x, 0, x, WINDOW_HEIGHT);
    }
    for (int j = 0; j <= boardSize; j++) {
        int y = (int)(j * cellHeight);
        SDL_RenderDrawLine(renderer, 0, y, WINDOW_WIDTH, y);
    }
}

// Fonction pour dessiner une lettre
void drawLetter(SDL_Renderer *renderer, TTF_Font *font, char letter, int x, int y, int boardSize) {
    if (letter == ' ' || letter == '\0') return;

    char text[2] = { letter, '\0' };
    if (strlen(input) == 0) {
    return; // Ne rien rendre si la chaîne est vide
}

    SDL_Surface *textSurface = TTF_RenderText_Blended(font, input, TEXT_COLOR);
    if (!textSurface) {
        fprintf(stderr, "Erreur TTF_RenderText_Blended: %s\n", TTF_GetError());
        return;
    }


    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        fprintf(stderr, "Erreur SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
        SDL_FreeSurface(textSurface);
        return;
    }

    float cellWidth  = (float)WINDOW_WIDTH  / boardSize;
    float cellHeight = (float)WINDOW_HEIGHT / boardSize;

    int cellX = (int)(x * cellWidth);
    int cellY = (int)(y * cellHeight);

    int textW, textH;
    SDL_QueryTexture(textTexture, NULL, NULL, &textW, &textH);

    int posX = cellX + (int)((cellWidth  - textW) / 2);
    int posY = cellY + (int)((cellHeight - textH) / 2);

    SDL_Rect dstRect = { posX, posY, textW, textH };

    SDL_RenderCopy(renderer, textTexture, NULL, &dstRect);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

// Fonction pour dessiner la zone d'entrée
void drawInputBox(SDL_Renderer *renderer, TTF_Font *font, const char *input) {
    SDL_Rect inputBox = { 50, WINDOW_HEIGHT - 100, WINDOW_WIDTH - 100, 50 };

    // Dessiner l'arrière-plan de la boîte
    SDL_SetRenderDrawColor(renderer, INPUT_BG_COLOR.r, INPUT_BG_COLOR.g, INPUT_BG_COLOR.b, INPUT_BG_COLOR.a);
    SDL_RenderFillRect(renderer, &inputBox);

    // Dessiner la bordure de la boîte
    SDL_SetRenderDrawColor(renderer, GRID_COLOR.r, GRID_COLOR.g, GRID_COLOR.b, GRID_COLOR.a);
    SDL_RenderDrawRect(renderer, &inputBox);

    // Dessiner le texte d'entrée
    SDL_Surface *textSurface = TTF_RenderText_Blended(font, input, TEXT_COLOR);
    if (!textSurface) {
        fprintf(stderr, "Erreur TTF_RenderText_Blended: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = { inputBox.x + 10, inputBox.y + 10, textSurface->w, textSurface->h };

    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

int main(int argc, char* argv[]) {
    int boardSize;
    printf("Entrez la taille du plateau (N) : ");
    if (scanf("%d", &boardSize) != 1 || boardSize <= 0) {
        fprintf(stderr, "Taille invalide.\n");
        return EXIT_FAILURE;
    }

    char **board = malloc(boardSize * sizeof(char*));
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

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Erreur SDL_Init: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "Erreur TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Window *window = SDL_CreateWindow("Scrabble Simplifie", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
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

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 32);
    if (!font) {
        fprintf(stderr, "Erreur TTF_OpenFont: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    bool quit = false;
    SDL_Event e;
    char inputText[50] = ""; // Texte en cours de saisie
    int selectedX = -1, selectedY = -1; // Case sélectionnée

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }

            if (e.type == SDL_TEXTINPUT) {
                if (strlen(inputText) < 49) {
                    strcat(inputText, e.text.text);
                }
            }

            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_BACKSPACE && strlen(inputText) > 0) {
                    inputText[strlen(inputText) - 1] = '\0';
                }
                if (e.key.keysym.sym == SDLK_RETURN && selectedX != -1 && selectedY != -1) {
                    for (int i = 0; i < strlen(inputText); i++) {
                        if (selectedX + i < boardSize) {
                            board[selectedY][selectedX + i] = toupper(inputText[i]);
                        }
                    }
                    inputText[0] = '\0'; // Réinitialiser la saisie
                }
            }

            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = e.button.x;
                int mouseY = e.button.y;

                selectedX = mouseX / (WINDOW_WIDTH / boardSize);
                selectedY = mouseY / (WINDOW_HEIGHT / boardSize);
            }
        }

        drawGrid(renderer, boardSize);

        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                drawLetter(renderer, font, board[y][x], x, y, boardSize);
            }
        }

        drawInputBox(renderer, font, inputText);

        SDL_RenderPresent(renderer);
    }

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

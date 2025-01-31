#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Largeur/hauteur de la fenêtre (en pixels)
#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 800

// Couleur de fond pour la grille
static SDL_Color BACKGROUND_COLOR = { 255, 255, 255, 255 };  // Blanc
// Couleur des lignes de la grille
static SDL_Color GRID_COLOR       = { 0,   0,   0,   255 };  // Noir
// Couleur du texte
static SDL_Color TEXT_COLOR       = { 0,   0,   0,   255 };  // Noir

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

// Fonction pour dessiner une lettre dans une case
void drawLetter(SDL_Renderer *renderer, TTF_Font *font, char letter, int x, int y, int boardSize) {
    if (letter == ' ' || letter == '\0') return;

    char text[2] = { letter, '\0' };

    SDL_Surface *textSurface = TTF_RenderText_Blended(font, text, TEXT_COLOR);
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

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 48);
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

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }

            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = e.button.x;
                int mouseY = e.button.y;

                int cellX = mouseX / (WINDOW_WIDTH / boardSize);
                int cellY = mouseY / (WINDOW_HEIGHT / boardSize);

                printf("Case cliquée : (%d, %d). Entrez une lettre ou un mot :\n", cellX, cellY);
                char input[50];
                scanf("%s", input);

                for (int i = 0; input[i] != '\0'; i++) {
                    input[i] = toupper(input[i]);
                }

                if (strlen(input) == 1) {
                    board[cellY][cellX] = input[0];
                } else {
                    printf("Entrez la direction (h ou v) : ");
                    char direction;
                    scanf(" %c", &direction);

                    bool valid = true;
                    int len = strlen(input);

                    if (direction == 'h') {
                        if (cellX + len > boardSize) valid = false;
                        for (int i = 0; i < len && valid; i++) {
                            if (board[cellY][cellX + i] != ' ' && board[cellY][cellX + i] != input[i]) valid = false;
                        }
                        if (valid) for (int i = 0; i < len; i++) board[cellY][cellX + i] = input[i];
                    } else if (direction == 'v') {
                        if (cellY + len > boardSize) valid = false;
                        for (int i = 0; i < len && valid; i++) {
                            if (board[cellY + i][cellX] != ' ' && board[cellY + i][cellX] != input[i]) valid = false;
                        }
                        if (valid) for (int i = 0; i < len; i++) board[cellY + i][cellX] = input[i];
                    }
                }
            }

            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                }
            }
        }

        drawGrid(renderer, boardSize);
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                drawLetter(renderer, font, board[y][x], x, y, boardSize);
            }
        }
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

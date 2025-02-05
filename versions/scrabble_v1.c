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
    // On efface l'écran avec la couleur de fond
    SDL_SetRenderDrawColor(renderer, 
                           BACKGROUND_COLOR.r, 
                           BACKGROUND_COLOR.g, 
                           BACKGROUND_COLOR.b, 
                           BACKGROUND_COLOR.a);
    SDL_RenderClear(renderer);

    // On dessine les lignes de la grille
    SDL_SetRenderDrawColor(renderer, 
                           GRID_COLOR.r, 
                           GRID_COLOR.g, 
                           GRID_COLOR.b, 
                           GRID_COLOR.a);

    // Largeur/hauteur d'une case (en pixels)
    float cellWidth  = (float)WINDOW_WIDTH  / boardSize;
    float cellHeight = (float)WINDOW_HEIGHT / boardSize;

    // Lignes verticales
    for (int i = 0; i <= boardSize; i++) {
        int x = (int)(i * cellWidth);
        SDL_RenderDrawLine(renderer, x, 0, x, WINDOW_HEIGHT);
    }
    // Lignes horizontales
    for (int j = 0; j <= boardSize; j++) {
        int y = (int)(j * cellHeight);
        SDL_RenderDrawLine(renderer, 0, y, WINDOW_WIDTH, y);
    }
}

// Fonction pour dessiner une lettre dans une case
void drawLetter(SDL_Renderer *renderer, TTF_Font *font, 
                char letter, int x, int y, int boardSize) 
{
    // Si la lettre est l'espace ' ' ou le caractère nul, on n'affiche rien
    if (letter == ' ' || letter == '\0') return;

    // On prépare la texture à partir de la lettre
    char text[2];
    text[0] = letter;
    text[1] = '\0';

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

    // Calcul de la position de la case dans la fenêtre
    float cellWidth  = (float)WINDOW_WIDTH  / boardSize;
    float cellHeight = (float)WINDOW_HEIGHT / boardSize;

    // Position (en pixels) du coin haut-gauche de la case
    int cellX = (int)(x * cellWidth);
    int cellY = (int)(y * cellHeight);

    // On veut centrer la lettre dans la case, on récupère la taille du texte
    int textW, textH;
    SDL_QueryTexture(textTexture, NULL, NULL, &textW, &textH);

    int posX = cellX + (int)((cellWidth  - textW) / 2);
    int posY = cellY + (int)((cellHeight - textH) / 2);

    // On définit le rectangle où coller la texture
    SDL_Rect dstRect;
    dstRect.x = posX;
    dstRect.y = posY;
    dstRect.w = textW;
    dstRect.h = textH;

    // On copie la texture dans le renderer
    SDL_RenderCopy(renderer, textTexture, NULL, &dstRect);

    // On libère les ressources temporaires
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

int main(int argc, char* argv[]) {
    // 1. Demander la taille du plateau
    int boardSize;
    printf("Entrez la taille du plateau (N) : ");
    if (scanf("%d", &boardSize) != 1 || boardSize <= 0) {
        fprintf(stderr, "Taille invalide.\n");
        return EXIT_FAILURE;
    }

    // 2. Créer un tableau 2D pour stocker les lettres (initialisé à l'espace ' ')
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

    // 3. Initialiser SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Erreur SDL_Init: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    // 4. Initialiser SDL_ttf
    if (TTF_Init() != 0) {
        fprintf(stderr, "Erreur TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // 5. Créer la fenêtre
    SDL_Window *window = SDL_CreateWindow("Scrabble Simplifie",
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

    // 6. Créer le renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Erreur SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // 7. Charger une police TTF
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

    // 8. Boucle principale
    while (!quit) {
        // a) On gère les événements SDL
        //    (Si l'utilisateur ferme la fenêtre, on quitte)
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            }
        }

        // b) On dessine la grille
        drawGrid(renderer, boardSize);

        // c) On dessine toutes les lettres déjà placées
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                drawLetter(renderer, font, board[y][x], x, y, boardSize);
            }
        }

        // d) On présente le rendu à l'écran
        SDL_RenderPresent(renderer);

        // e) On demande une saisie à l'utilisateur (coordonnées + lettre)
        //    Attention, cette partie bloque la boucle : la fenêtre ne se mettra plus à jour
        //    tant que l'utilisateur n'a pas entré ses coordonnées dans la console.
        printf("Entrez une coordonnee (x y) et une lettre (ex: 2 3 A), ou -1 pour quitter.\n");
printf("Pour entrer un mot : x y sens(h/v) mot (ex: 2 3 h TEST).\n");
int x, y;
char c;
char direction; // Pour 'h' ou 'v'
char word[50];  // Pour un mot entier

int ret = scanf("%d", &x);
if (ret != 1) {
    printf("Saisie invalide, on quitte.\n");
    quit = true;
} else {
    if (x == -1) {
        quit = true;
    } else {
        if (scanf("%d", &y) != 1) {
            printf("Saisie invalide, on quitte.\n");
            quit = true;
        } else {
            // Si un mot entier est entré (avec direction)
            ret = scanf(" %c", &direction);
            if (ret == 1 && (direction == 'h' || direction == 'v')) {
                if (scanf(" %s", word) == 1) {
                    // Convertir le mot en majuscules
                    for (int i = 0; word[i] != '\0'; i++) {
                        word[i] = toupper(word[i]);
                    }

                    // Place le mot s'il peut tenir dans la grille
                    bool valid = true;
                    int len = strlen(word);


                    if (direction == 'h') { // Horizontal
                        if (x + len > boardSize) {
                            printf("Le mot dépasse de la grille horizontalement.\n");
                            valid = false;
                        } else {
                            for (int i = 0; i < len; i++) {
                                if (board[y][x + i] != ' ' && board[y][x + i] != word[i]) {
                                    printf("Collision détectée avec un autre mot.\n");
                                    valid = false;
                                    break;
                                }
                            }
                        }
                    } else if (direction == 'v') { // Vertical
                        if (y + len > boardSize) {
                            printf("Le mot dépasse de la grille verticalement.\n");
                            valid = false;
                        } else {
                            for (int i = 0; i < len; i++) {
                                if (board[y + i][x] != ' ' && board[y + i][x] != word[i]) {
                                    printf("Collision détectée avec un autre mot.\n");
                                    valid = false;
                                    break;
                                }
                            }
                        }
                    }

                    // S                                                                    i tout est valide, placer le mot
                    if (valid) {
                        for (int i = 0; i < len; i++) {
                            if (direction == 'h') {
                                board[y][x + i] = word[i];
                            } else if (direction == 'v') {
                                board[y + i][x] = word[i];
                            }
                        }
                    }
                }
            } else {
                // Sinon, gestion classique pour une seule lettre
                if (scanf(" %c", &c) == 1) {
                    if (x >= 0 && x < boardSize && y >= 0 && y < boardSize) {
                        board[y][x] = c;
                    } else {
                        printf("Coordonnees hors de la grille.\n");
                    }
                }
            }
        }
    }
}

    }

    // 9. Nettoyage final
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

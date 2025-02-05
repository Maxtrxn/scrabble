#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#define WINDOW_WIDTH      800
#define WINDOW_HEIGHT     900

#define BOARD_HEIGHT      800
#define RACK_HEIGHT       50
#define INPUT_AREA_HEIGHT (WINDOW_HEIGHT - BOARD_HEIGHT - RACK_HEIGHT)

// Marge autour de la zone de dessin du plateau
#define BOARD_MARGIN      50

// Couleurs générales
static SDL_Color BACKGROUND_COLOR = {255, 255, 255, 255};  // Blanc
static SDL_Color GRID_COLOR       = {255, 255, 255, 255};  // Blanc
static SDL_Color TEXT_COLOR       = {0, 0, 0, 255};        // Noir
static SDL_Color INPUT_BG_COLOR   = {200, 200, 200, 255};  // Gris clair

// États de saisie
typedef enum {
    STATE_IDLE,
    STATE_INPUT_TEXT,
    STATE_INPUT_DIRECTION
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

// Fonction pour tirer une lettre aléatoire selon la distribution classique du Scrabble (sans jokers)
char drawRandomLetter() {
    struct { char letter; int count; } distribution[] = {
        {'A', 9}, {'B', 2}, {'C', 2}, {'D', 4}, {'E', 12},
        {'F', 2}, {'G', 3}, {'H', 2}, {'I', 9}, {'J', 1},
        {'K', 1}, {'L', 4}, {'M', 2}, {'N', 6}, {'O', 8},
        {'P', 2}, {'Q', 1}, {'R', 6}, {'S', 4}, {'T', 6},
        {'U', 4}, {'V', 2}, {'W', 2}, {'X', 1}, {'Y', 2},
        {'Z', 1}
    };
    int total = 0;
    for (int i = 0; i < 26; i++) {
        total += distribution[i].count;
    }
    int r = rand() % total;
    for (int i = 0; i < 26; i++) {
        if (r < distribution[i].count)
            return distribution[i].letter;
        r -= distribution[i].count;
    }
    return 'A';
}

// Vérifie (sans modifier) si le mot peut être formé avec le rack
bool canFormWordFromRack(const char *word, const char *rack) {
    char temp[8];
    strncpy(temp, rack, 8);
    int len = strlen(word);
    for (int i = 0; i < len; i++) {
        char c = word[i];
        bool found = false;
        for (int j = 0; j < 7; j++) {
            if (temp[j] == c) {
                temp[j] = '#';
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

// Consomme les lettres du rack (si le mot est formable) et met à jour le rack
bool consumeRackLetters(const char *word, char *rack) {
    if (!canFormWordFromRack(word, rack))
        return false;
    char temp[8];
    strncpy(temp, rack, 8);
    int len = strlen(word);
    for (int i = 0; i < len; i++) {
        char c = word[i];
        for (int j = 0; j < 7; j++) {
            if (temp[j] == c) {
                temp[j] = '#';
                break;
            }
        }
    }
    for (int j = 0; j < 7; j++) {
        if (temp[j] == '#')
            temp[j] = drawRandomLetter();
    }
    strcpy(rack, temp);
    return true;
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    int boardSize = 15;
    // Allocation du plateau
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

    // Plateau des bonus (standard Scrabble)
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

    // Variable pour la somme totale des points
    int totalPoints = 0;

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

    // Chargement des polices
    TTF_Font *boardFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 28);
    if (!boardFont) {
        fprintf(stderr, "Erreur TTF_OpenFont (boardFont): %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
    // Pour le rack, on utilise une police plus petite (taille 20)
    TTF_Font *rackFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20);
    if (!rackFont) {
        fprintf(stderr, "Erreur TTF_OpenFont (rackFont): %s\n", TTF_GetError());
        TTF_CloseFont(boardFont);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
    TTF_Font *inputFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
    if (!inputFont) {
        fprintf(stderr, "Erreur TTF_OpenFont (inputFont): %s\n", TTF_GetError());
        TTF_CloseFont(rackFont);
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
        TTF_CloseFont(rackFont);
        TTF_CloseFont(boardFont);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Initialisation du rack (chevalet) avec 7 lettres aléatoires
    char rack[8];
    for (int i = 0; i < 7; i++) {
        rack[i] = drawRandomLetter();
    }
    rack[7] = '\0';

    // Variables de gestion de la saisie
    InputState currentState = STATE_IDLE;
    char inputBuffer[50] = "";
    int inputLength = 0;
    char tempWord[50] = "";
    int selectedCellX = -1, selectedCellY = -1;
    int lastWordScore = 0;

    bool quit = false;
    SDL_Event e;

    // Calcul des dimensions du plateau
    int boardDrawWidth  = WINDOW_WIDTH - 2 * BOARD_MARGIN;
    int boardDrawHeight = BOARD_HEIGHT - 2 * BOARD_MARGIN;
    float cellWidth  = (float)boardDrawWidth / boardSize;
    float cellHeight = (float)boardDrawHeight / boardSize;
    int gridThickness = 2;  // Épaisseur de la grille

    // Définition de la zone du rack : on fixe une largeur de 300 pixels pour le rack,
    // et le bouton rouge sera placé juste après.
    int rackAreaWidth = 300;
    float rackCellWidth = (float)rackAreaWidth / 7;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = true;
            if (currentState == STATE_IDLE) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int mouseX = e.button.x;
                    int mouseY = e.button.y;
                    // Clic dans la zone du plateau
                    if (mouseX >= BOARD_MARGIN && mouseX < (BOARD_MARGIN + boardDrawWidth) &&
                        mouseY >= BOARD_MARGIN && mouseY < (BOARD_MARGIN + boardDrawHeight)) {
                        selectedCellX = (mouseX - BOARD_MARGIN) / cellWidth;
                        selectedCellY = (mouseY - BOARD_MARGIN) / cellHeight;
                        currentState = STATE_INPUT_TEXT;
                        inputBuffer[0] = '\0';
                        inputLength = 0;
                        SDL_StartTextInput();
                    }
                    // Clic dans la zone du rack
                    else if (mouseY >= BOARD_HEIGHT && mouseY < (BOARD_HEIGHT + RACK_HEIGHT)) {
                        // Bouton rouge : calcul du rectangle du bouton
                        SDL_Surface *btnSurface = TTF_RenderText_Blended(inputFont, "Echanger", TEXT_COLOR);
                        int btnW, btnH;
                        SDL_Texture *tempTex = SDL_CreateTextureFromSurface(renderer, btnSurface);
                        SDL_QueryTexture(tempTex, NULL, NULL, &btnW, &btnH);
                        SDL_DestroyTexture(tempTex);
                        SDL_FreeSurface(btnSurface);
                        int buttonX = BOARD_MARGIN + rackAreaWidth + 10; // 10px de marge après le rack
                        int buttonY = BOARD_HEIGHT + (RACK_HEIGHT - btnH - 4) / 2;
                        int buttonWidth = btnW + 10; // 10px de marge interne
                        int buttonHeight = btnH + 4;  // 4px de marge interne
                        if (mouseX >= buttonX && mouseX < buttonX + buttonWidth &&
                            mouseY >= buttonY && mouseY < buttonY + buttonHeight) {
                            // Bouton cliqué : remplace le rack par 7 nouvelles lettres
                            for (int i = 0; i < 7; i++) {
                                rack[i] = drawRandomLetter();
                            }
                            rack[7] = '\0';
                        }
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
                            // Vérifie que la lettre est présente dans le rack
                            if (canFormWordFromRack(inputBuffer, rack)) {
                                consumeRackLetters(inputBuffer, rack);
                                int letterScore = getLetterScore(inputBuffer[0]);
                                int bonus = bonusBoard[selectedCellY][selectedCellX];
                                if (selectedCellX == 7 && selectedCellY == 7) bonus = 2;
                                if (bonus == 3) letterScore *= 3;
                                else if (bonus == 4) letterScore *= 2;
                                else if (bonus == 1) letterScore *= 3;
                                else if (bonus == 2) letterScore *= 2;
                                lastWordScore = letterScore;
                                totalPoints += letterScore;
                                board[selectedCellY][selectedCellX] = inputBuffer[0];
                            }
                            currentState = STATE_IDLE;
                        }
                        else {
                            // Pour un mot complet, vérifie que toutes les lettres sont dans le rack
                            if (canFormWordFromRack(inputBuffer, rack)) {
                                consumeRackLetters(inputBuffer, rack);
                                strcpy(tempWord, inputBuffer);
                                inputBuffer[0] = '\0';
                                inputLength = 0;
                                currentState = STATE_INPUT_DIRECTION;
                            } else {
                                // Annule si le mot contient des lettres absentes du rack
                                currentState = STATE_IDLE;
                            }
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
                                totalPoints += computedScore;
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
                                totalPoints += computedScore;
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

        // Fond général
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
                if (x == 7 && y == 7) {
                    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255); // Case centrale en doré
                }
                else if (bonusBoard[y][x] == 1) {
                    SDL_SetRenderDrawColor(renderer, 200, 39, 34, 255); // Triple mot : Rouge
                }
                else if (bonusBoard[y][x] == 2) {
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);  // Double mot : Orange
                }
                else if (bonusBoard[y][x] == 3) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);    // Triple lettre : Bleu
                }
                else if (bonusBoard[y][x] == 4) {
                    SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255);  // Double lettre : Bleu clair
                }
                else {
                    SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);    // Case normale : Vert neutre
                }
                SDL_RenderFillRect(renderer, &cellRect);
            }
        }

        // 1.5. Dessin du petit carré beige sur les cases où une lettre a été placée
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
                    SDL_SetRenderDrawColor(renderer, 245, 245, 220, 255); // Carré beige opaque
                    SDL_RenderFillRect(renderer, &overlayRect);
                }
            }
        }

        // 2. Dessin de la grille
        SDL_SetRenderDrawColor(renderer, GRID_COLOR.r, GRID_COLOR.g, GRID_COLOR.b, GRID_COLOR.a);
        for (int i = 0; i <= boardSize; i++) {
            int x = BOARD_MARGIN + (int)(i * cellWidth);
            for (int offset = 0; offset < gridThickness; offset++) {
                SDL_RenderDrawLine(renderer, x + offset, BOARD_MARGIN, x + offset, BOARD_MARGIN + boardDrawHeight);
            }
        }
        for (int j = 0; j <= boardSize; j++) {
            int y = BOARD_MARGIN + (int)(j * cellHeight);
            for (int offset = 0; offset < gridThickness; offset++) {
                SDL_RenderDrawLine(renderer, BOARD_MARGIN, y + offset, BOARD_MARGIN + boardDrawWidth, y + offset);
            }
        }

        // 3. Dessin des lettres sur la grille et de leur valeur (la valeur en bas à droite)
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
                    // Affichage de la valeur de la lettre dans le coin inférieur droit (avec valueFont)
                    char valueText[4];
                    sprintf(valueText, "%d", getLetterScore(letter));
                    SDL_Surface *valueSurface = TTF_RenderText_Blended(valueFont, valueText, TEXT_COLOR);
                    if (valueSurface) {
                        SDL_Texture *valueTexture = SDL_CreateTextureFromSurface(renderer, valueSurface);
                        int valueW, valueH;
                        SDL_QueryTexture(valueTexture, NULL, NULL, &valueW, &valueH);
                        int valuePosX = BOARD_MARGIN + (int)(x * cellWidth) + (int)cellWidth - valueW - 2;
                        int valuePosY = BOARD_MARGIN + (int)(y * cellHeight) + (int)cellHeight - valueH - 2;
                        SDL_Rect valueRect = { valuePosX, valuePosY, valueW, valueH };
                        SDL_RenderCopy(renderer, valueTexture, NULL, &valueRect);
                        SDL_DestroyTexture(valueTexture);
                        SDL_FreeSurface(valueSurface);
                    }
                }
            }
        }

        // 4. Dessin du chevalet (rack) entre la grille et la zone de saisie
        // Le rack occupe la zone de BOARD_HEIGHT à BOARD_HEIGHT+RACK_HEIGHT, sur la partie gauche (largeur rackAreaWidth)
        int rackAreaWidth = 300;  // Zone réservée pour le rack
        SDL_Rect rackRect = { BOARD_MARGIN, BOARD_HEIGHT, rackAreaWidth, RACK_HEIGHT };
        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
        SDL_RenderFillRect(renderer, &rackRect);
        // Affichage des lettres du rack avec rackFont
        for (int i = 0; i < 7; i++) {
            char letter[2] = { rack[i], '\0' };
            SDL_Surface *rackSurface = TTF_RenderText_Blended(rackFont, letter, TEXT_COLOR);
            if (rackSurface) {
                SDL_Texture *rackTexture = SDL_CreateTextureFromSurface(renderer, rackSurface);
                int rackW, rackH;
                SDL_QueryTexture(rackTexture, NULL, NULL, &rackW, &rackH);
                int cellX = BOARD_MARGIN + i * rackCellWidth;
                int cellY = BOARD_HEIGHT;
                int drawX = cellX + (int)((rackCellWidth - rackW) / 2);
                int drawY = cellY + (int)((RACK_HEIGHT - rackH) / 2);
                SDL_Rect dstRect = { drawX, drawY, rackW, rackH };
                SDL_RenderCopy(renderer, rackTexture, NULL, &dstRect);
                SDL_DestroyTexture(rackTexture);
                SDL_FreeSurface(rackSurface);
            }
        }
        // Dessin du bouton rouge à droite du rack
        SDL_Surface *btnSurface = TTF_RenderText_Blended(inputFont, "Echanger", TEXT_COLOR);
        int btnW, btnH;
        SDL_Texture *tempTex = SDL_CreateTextureFromSurface(renderer, btnSurface);
        SDL_QueryTexture(tempTex, NULL, NULL, &btnW, &btnH);
        SDL_DestroyTexture(tempTex);
        SDL_FreeSurface(btnSurface);
        int buttonWidth = btnW + 10; // juste assez pour le texte plus une petite marge
        int buttonHeight = btnH + 4;
        int buttonX = BOARD_MARGIN + rackAreaWidth + 10;
        int buttonY = BOARD_HEIGHT + (RACK_HEIGHT - buttonHeight) / 2;
        SDL_Rect buttonRect = { buttonX, buttonY, buttonWidth, buttonHeight };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &buttonRect);
        // Texte sur le bouton
        btnSurface = TTF_RenderText_Blended(inputFont, "Echanger", TEXT_COLOR);
        if (btnSurface) {
            SDL_Texture *btnTexture = SDL_CreateTextureFromSurface(renderer, btnSurface);
            SDL_QueryTexture(btnTexture, NULL, NULL, &btnW, &btnH);
            int btnTextX = buttonX + (buttonWidth - btnW) / 2;
            int btnTextY = buttonY + (buttonHeight - btnH) / 2;
            SDL_Rect btnRect = { btnTextX, btnTextY, btnW, btnH };
            SDL_RenderCopy(renderer, btnTexture, NULL, &btnRect);
            SDL_DestroyTexture(btnTexture);
            SDL_FreeSurface(btnSurface);
        }

        // 5. Dessin de la zone de saisie (en bas)
        SDL_Rect inputRect = { 0, BOARD_HEIGHT + RACK_HEIGHT, WINDOW_WIDTH, INPUT_AREA_HEIGHT };
        SDL_SetRenderDrawColor(renderer, INPUT_BG_COLOR.r, INPUT_BG_COLOR.g, INPUT_BG_COLOR.b, INPUT_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &inputRect);
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
            int posY = BOARD_HEIGHT + RACK_HEIGHT + (INPUT_AREA_HEIGHT - textH) / 2;
            SDL_Rect dstRect = { posX, posY, textW, textH };
            SDL_RenderCopy(renderer, promptTexture, NULL, &dstRect);
            SDL_DestroyTexture(promptTexture);
            SDL_FreeSurface(promptSurface);
        }

        // 6. Affichage du score du dernier mot dans le coin supérieur droit
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

        // 7. Affichage du total des points en haut à gauche
        char totalText[50];
        snprintf(totalText, sizeof(totalText), "Total: %d", totalPoints);
        SDL_Surface *totalSurface = TTF_RenderText_Blended(boardFont, totalText, TEXT_COLOR);
        if (totalSurface) {
            SDL_Texture *totalTexture = SDL_CreateTextureFromSurface(renderer, totalSurface);
            int totalW, totalH;
            SDL_QueryTexture(totalTexture, NULL, NULL, &totalW, &totalH);
            int posX_total = 10;
            int posY_total = 10;
            SDL_Rect totalRect = { posX_total, posY_total, totalW, totalH };
            SDL_RenderCopy(renderer, totalTexture, NULL, &totalRect);
            SDL_DestroyTexture(totalTexture);
            SDL_FreeSurface(totalSurface);
        }

        // 8. Dessin final des lettres sur la grille et de leur valeur (valeurs en bas à droite)
        // On redessine les lettres et leurs valeurs pour mettre à jour la position des valeurs (en bas à droite)
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                char letter = board[y][x];
                if (letter != ' ') {
                    // Lettre principale (centrée)
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
                    // Valeur de la lettre (coin inférieur droit)
                    char valueText[4];
                    sprintf(valueText, "%d", getLetterScore(letter));
                    SDL_Surface *valueSurface = TTF_RenderText_Blended(valueFont, valueText, TEXT_COLOR);
                    if (valueSurface) {
                        SDL_Texture *valueTexture = SDL_CreateTextureFromSurface(renderer, valueSurface);
                        int valueW, valueH;
                        SDL_QueryTexture(valueTexture, NULL, NULL, &valueW, &valueH);
                        int valuePosX = BOARD_MARGIN + (int)(x * cellWidth) + (int)cellWidth - valueW - 2;
                        int valuePosY = BOARD_MARGIN + (int)(y * cellHeight) + (int)cellHeight - valueH - 2;
                        SDL_Rect valueRect = { valuePosX, valuePosY, valueW, valueH };
                        SDL_RenderCopy(renderer, valueTexture, NULL, &valueRect);
                        SDL_DestroyTexture(valueTexture);
                        SDL_FreeSurface(valueSurface);
                    }
                }
            }
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
    TTF_CloseFont(inputFont);
    TTF_CloseFont(rackFont);
    TTF_CloseFont(boardFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}


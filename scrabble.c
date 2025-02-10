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

#define BOARD_MARGIN      50

// Couleurs
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

// --- Fonctions pour le Scrabble ---

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

// Tire une lettre aléatoire selon la distribution classique du Scrabble (sans jokers)
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
    for (int i = 0; i < 26; i++)
        total += distribution[i].count;
    int r = rand() % total;
    for (int i = 0; i < 26; i++) {
        if (r < distribution[i].count)
            return distribution[i].letter;
        r -= distribution[i].count;
    }
    return 'A';
}

// Vérifie si le mot peut être placé à partir de (startX, startY) dans la direction dir ('h' ou 'v'),
// en utilisant les lettres du rack pour les cases vides et en acceptant celles déjà présentes.
// Si totalPoints > 0 (après le premier coup), le mot doit intersecter au moins une lettre déjà placée.
// Pour le premier coup, la case centrale doit être utilisée.
bool canPlaceWord(const char *word, int startX, int startY, char dir,
                  char **board, int boardSize, const char *rack, int totalPoints) {
    bool intersects = false;
    int freq[26] = {0};
    for (int i = 0; i < 7; i++) {
        char c = rack[i];
        if (c >= 'A' && c <= 'Z')
            freq[c - 'A']++;
    }
    int len = strlen(word);
    for (int i = 0; i < len; i++) {
        int x = startX, y = startY;
        if (dir == 'h') x += i; else y += i;
        if (x < 0 || x >= boardSize || y < 0 || y >= boardSize)
            return false;
        char boardLetter = board[y][x];
        char wordLetter = toupper(word[i]);
        if (boardLetter == ' ') {
            if (freq[wordLetter - 'A'] <= 0)
                return false;
            freq[wordLetter - 'A']--;
        } else {
            if (toupper(boardLetter) != wordLetter)
                return false;
            intersects = true;
        }
    }
    if (totalPoints > 0 && !intersects)
        return false;
    if (totalPoints == 0) {
        if (startX != boardSize / 2 || startY != boardSize / 2)
            return false;
    }
    return true;
}

// Place le mot sur le plateau à partir de (startX, startY) dans la direction dir ('h' ou 'v').
// Pour chaque case vide, la lettre est placée et la lettre correspondante est consommée du rack.
void placeWord(const char *word, int startX, int startY, char dir,
               char **board, int boardSize, char *rack) {
    int len = strlen(word);
    for (int i = 0; i < len; i++) {
        int x = startX, y = startY;
        if (dir == 'h') x += i; else y += i;
        if (board[y][x] == ' ') {
            board[y][x] = toupper(word[i]);
            // Consomme la lettre du rack uniquement pour les cases vides
            for (int j = 0; j < 7; j++) {
                if (toupper(rack[j]) == toupper(word[i])) {
                    rack[j] = drawRandomLetter();
                    break;
                }
            }
        }
    }
}

// --- Fonctions pour la gestion du dictionnaire avec recherche dichotomique ---

// Comparaison insensible à la casse utilisée pour le tri
int caseInsensitiveCompare(const void *a, const void *b) {
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcasecmp(str1, str2);
}

// Charge le dictionnaire depuis le fichier "mot_filtrés.txt" et retourne un tableau de chaînes trié.
// Le nombre de mots chargés est retourné dans outCount.
char **loadDictionary(const char *filename, int *outCount) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s\n", filename);
        return NULL;
    }
    int capacity = 100;
    int count = 0;
    char **words = malloc(capacity * sizeof(char *));
    if (!words) {
        fclose(fp);
        return NULL;
    }
    char buffer[100];
    while (fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\n")] = '\0'; // Supprime le saut de ligne
        words[count] = strdup(buffer);
        if (!words[count]) {
            // En cas d'erreur, libérer la mémoire allouée
            for (int i = 0; i < count; i++)
                free(words[i]);
            free(words);
            fclose(fp);
            return NULL;
        }
        count++;
        if (count >= capacity) {
            capacity *= 2;
            char **temp = realloc(words, capacity * sizeof(char *));
            if (!temp) {
                for (int i = 0; i < count; i++)
                    free(words[i]);
                free(words);
                fclose(fp);
                return NULL;
            }
            words = temp;
        }
    }
    fclose(fp);
    // Trie le tableau de mots de manière insensible à la casse
    qsort(words, count, sizeof(char *), caseInsensitiveCompare);
    *outCount = count;
    return words;
}

// Recherche dichotomique dans le tableau trié
int binarySearch(char **array, int size, const char *target) {
    int low = 0;
    int high = size - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        int cmp = strcasecmp(array[mid], target);
        if (cmp == 0)
            return mid;  // Trouvé
        else if (cmp < 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return -1; // Non trouvé
}

// Vérifie si le mot est valide en recherchant dans le dictionnaire chargé
bool isValidWordBinary(const char *word, char **dictionary, int dictionaryCount) {
    return binarySearch(dictionary, dictionaryCount, word) >= 0;
}

// --- Programme principal ---
int main(int argc, char* argv[]) {
    srand(time(NULL));

    // Chargement du dictionnaire (à partir du fichier "mots_filtrés.txt")
    int dictionaryCount = 0;
    char **dictionary = loadDictionary("mots_filtrés.txt", &dictionaryCount);
    if (!dictionary) {
        fprintf(stderr, "Erreur lors du chargement du dictionnaire.\n");
        return EXIT_FAILURE;
    }

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
    int selectedCellX = -1, selectedCellY = -1;
    int lastWordScore = 0;

    bool quit = false;
    SDL_Event e;

    // Calcul des dimensions du plateau
    int boardDrawWidth  = WINDOW_WIDTH - 2 * BOARD_MARGIN;
    int boardDrawHeight = BOARD_HEIGHT - 2 * BOARD_MARGIN;
    float cellWidth  = (float)boardDrawWidth / boardSize;
    float cellHeight = (float)boardDrawHeight / boardSize;
    int gridThickness = 2;

    // Zone du rack et bouton "Echanger"
    int rackAreaWidth = 300;  // Largeur réservée pour le rack
    float rackCellWidth = (float)rackAreaWidth / 7;
    SDL_Surface *btnSurface = TTF_RenderText_Blended(inputFont, "Echanger", TEXT_COLOR);
    int btnW, btnH;
    SDL_Texture *tempTex = SDL_CreateTextureFromSurface(renderer, btnSurface);
    SDL_QueryTexture(tempTex, NULL, NULL, &btnW, &btnH);
    SDL_DestroyTexture(tempTex);
    SDL_FreeSurface(btnSurface);
    int buttonWidth = btnW + 10;
    int buttonHeight = btnH + 4;
    int buttonMargin = 10;
    int totalRackWidth = rackAreaWidth + buttonMargin + buttonWidth;
    int startXRack = (WINDOW_WIDTH - totalRackWidth) / 2; // Centre la zone rack + bouton

    // Forçage du premier mot sur la case centrale dès le lancement
    selectedCellX = boardSize / 2;
    selectedCellY = boardSize / 2;
    currentState = STATE_INPUT_TEXT;
    SDL_StartTextInput();

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
                        int buttonX = startXRack + rackAreaWidth + buttonMargin;
                        int buttonY = BOARD_HEIGHT + (RACK_HEIGHT - buttonHeight) / 2;
                        if (mouseX >= buttonX && mouseX < buttonX + buttonWidth &&
                            mouseY >= buttonY && mouseY < buttonY + buttonHeight) {
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
                        // Utilisation de la recherche dichotomique dans le dictionnaire chargé
                        else if (!isValidWordBinary(inputBuffer, dictionary, dictionaryCount)) {
                            // Le mot n'est pas valide : on réinitialise la saisie sans rien placer.
                            fprintf(stderr, "Mot invalide: %s\n", inputBuffer);
                            inputBuffer[0] = '\0';
                            inputLength = 0;
                            currentState = STATE_IDLE;
                        }
                        else if (inputLength == 1) {
                            // Pour un mot d'une seule lettre, on suppose l'orientation horizontale par défaut.
                            if (canPlaceWord(inputBuffer, selectedCellX, selectedCellY, 'h', board, boardSize, rack, totalPoints)) {
                                placeWord(inputBuffer, selectedCellX, selectedCellY, 'h', board, boardSize, rack);
                                int letterScore = getLetterScore(inputBuffer[0]);
                                lastWordScore = letterScore;
                                totalPoints += letterScore;
                            }
                            currentState = STATE_IDLE;
                        }
                        else {
                            // Pour un mot de plusieurs lettres, on passe en mode demande de direction.
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
                        if (canPlaceWord(inputBuffer, selectedCellX, selectedCellY, dir, board, boardSize, rack, totalPoints)) {
                            placeWord(inputBuffer, selectedCellX, selectedCellY, dir, board, boardSize, rack);
                            int score = 0;
                            int len = strlen(inputBuffer);
                            for (int i = 0; i < len; i++) {
                                int x = selectedCellX, y = selectedCellY;
                                if (dir == 'h') x += i; else y += i;
                                if (board[y][x] == toupper(inputBuffer[i])) {
                                    score += getLetterScore(inputBuffer[i]);
                                }
                            }
                            lastWordScore = score;
                            totalPoints += score;
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

        // 1. Plateau et bonus
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                SDL_Rect cellRect = {
                    BOARD_MARGIN + (int)(x * cellWidth),
                    BOARD_MARGIN + (int)(y * cellHeight),
                    (int)cellWidth,
                    (int)cellHeight
                };
                if (x == boardSize/2 && y == boardSize/2)
                    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
                else if (bonusBoard[y][x] == 1)
                    SDL_SetRenderDrawColor(renderer, 200, 39, 34, 255);
                else if (bonusBoard[y][x] == 2)
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
                else if (bonusBoard[y][x] == 3)
                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                else if (bonusBoard[y][x] == 4)
                    SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255);
                else
                    SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
                SDL_RenderFillRect(renderer, &cellRect);
            }
        }

        // 1.5. Carrés beige sur le plateau (pour chaque lettre placée)
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
                    SDL_SetRenderDrawColor(renderer, 245, 245, 220, 255);
                    SDL_RenderFillRect(renderer, &overlayRect);
                }
            }
        }

        // 2. Grille
        SDL_SetRenderDrawColor(renderer, GRID_COLOR.r, GRID_COLOR.g, GRID_COLOR.b, GRID_COLOR.a);
        for (int i = 0; i <= boardSize; i++) {
            int x = BOARD_MARGIN + (int)(i * cellWidth);
            for (int offset = 0; offset < gridThickness; offset++)
                SDL_RenderDrawLine(renderer, x + offset, BOARD_MARGIN, x + offset, BOARD_MARGIN + boardDrawHeight);
        }
        for (int j = 0; j <= boardSize; j++) {
            int y = BOARD_MARGIN + (int)(j * cellHeight);
            for (int offset = 0; offset < gridThickness; offset++)
                SDL_RenderDrawLine(renderer, BOARD_MARGIN, y + offset, BOARD_MARGIN + boardDrawWidth, y + offset);
        }

        // 3. Lettres et valeurs sur le plateau (valeur en bas à droite)
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                char letter = board[y][x];
                if (letter != ' ') {
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

        // 4. Zone du chevalet et bouton "Echanger" (centré)
        totalRackWidth = rackAreaWidth + buttonMargin + buttonWidth;
        startXRack = (WINDOW_WIDTH - totalRackWidth) / 2;
        // Zone du rack
        SDL_Rect rackRect = { startXRack, BOARD_HEIGHT, rackAreaWidth, RACK_HEIGHT };
        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
        SDL_RenderFillRect(renderer, &rackRect);
        // Pour chaque jeton, dessin d'un carré beige, de la lettre et de sa valeur (en bas à droite)
        for (int i = 0; i < 7; i++) {
            float currentCellWidth = rackAreaWidth / 7.0;
            int tileWidth = (int)round(currentCellWidth * 0.8);
            int tileHeight = (int)round(RACK_HEIGHT * 0.8);
            int tileOffsetX = (int)round((currentCellWidth - tileWidth) / 2.0);
            int tileOffsetY = (int)round((RACK_HEIGHT - tileHeight) / 2.0);
            int cellX = startXRack + (int)(i * currentCellWidth);
            int cellY = BOARD_HEIGHT;
            SDL_Rect tileRect = { cellX + tileOffsetX, cellY + tileOffsetY, tileWidth, tileHeight };
            SDL_SetRenderDrawColor(renderer, 245, 245, 220, 255);
            SDL_RenderFillRect(renderer, &tileRect);
            // Lettre du jeton
            char letter[2] = { rack[i], '\0' };
            SDL_Surface *rackSurface = TTF_RenderText_Blended(rackFont, letter, TEXT_COLOR);
            if (rackSurface) {
                SDL_Texture *rackTexture = SDL_CreateTextureFromSurface(renderer, rackSurface);
                int rackW, rackH;
                SDL_QueryTexture(rackTexture, NULL, NULL, &rackW, &rackH);
                int drawX = cellX + tileOffsetX + (tileWidth - rackW) / 2;
                int drawY = cellY + tileOffsetY + (tileHeight - rackH) / 2;
                SDL_Rect dstRect = { drawX, drawY, rackW, rackH };
                SDL_RenderCopy(renderer, rackTexture, NULL, &dstRect);
                SDL_DestroyTexture(rackTexture);
                SDL_FreeSurface(rackSurface);
            }
            // Valeur dans le coin inférieur droit du jeton
            char valueText[4];
            sprintf(valueText, "%d", getLetterScore(rack[i]));
            SDL_Surface *valueSurface = TTF_RenderText_Blended(valueFont, valueText, TEXT_COLOR);
            if (valueSurface) {
                SDL_Texture *valueTexture = SDL_CreateTextureFromSurface(renderer, valueSurface);
                int valueW, valueH;
                SDL_QueryTexture(valueTexture, NULL, NULL, &valueW, &valueH);
                int valuePosX = cellX + tileOffsetX + tileWidth - valueW - 2;
                int valuePosY = cellY + tileOffsetY + tileHeight - valueH - 2;
                SDL_Rect valueRect = { valuePosX, valuePosY, valueW, valueH };
                SDL_RenderCopy(renderer, valueTexture, NULL, &valueRect);
                SDL_DestroyTexture(valueTexture);
                SDL_FreeSurface(valueSurface);
            }
        }
        // Bouton "Echanger"
        int buttonX = startXRack + rackAreaWidth + buttonMargin;
        int buttonY = BOARD_HEIGHT + (RACK_HEIGHT - buttonHeight) / 2;
        SDL_Rect buttonRect = { buttonX, buttonY, buttonWidth, buttonHeight };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &buttonRect);
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

        // 5. Zone de saisie (en bas)
        SDL_Rect inputRect = { 0, BOARD_HEIGHT + RACK_HEIGHT, WINDOW_WIDTH, INPUT_AREA_HEIGHT };
        SDL_SetRenderDrawColor(renderer, INPUT_BG_COLOR.r, INPUT_BG_COLOR.g, INPUT_BG_COLOR.b, INPUT_BG_COLOR.a);
        SDL_RenderFillRect(renderer, &inputRect);
        char displayText[100];
        if (currentState == STATE_IDLE) {
            snprintf(displayText, sizeof(displayText), "Entrez un mot (premier mot d%cj%c sur la case centrale)",130,133);
            /* 130 = é
              133 = à
              138 = è
              135 = ç
              136 = ê  */
        }
        else if (currentState == STATE_INPUT_TEXT) {
            snprintf(displayText, sizeof(displayText), "Entrez un mot: %s", inputBuffer);
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

        // 8. Redessin final des lettres sur le plateau et de leur valeur (valeur en bas à droite)
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                char letter = board[y][x];
                if (letter != ' ') {
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
        SDL_Delay(16);
    }

    // Libération de la mémoire du plateau
    for (int i = 0; i < boardSize; i++)
        free(board[i]);
    free(board);

    // Libération de la mémoire du dictionnaire
    for (int i = 0; i < dictionaryCount; i++) {
        free(dictionary[i]);
    }
    free(dictionary);

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

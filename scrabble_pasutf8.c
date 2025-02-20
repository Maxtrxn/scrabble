// Définition de la version POSIX (pour avoir accès à certaines fonctionnalités)
#define _POSIX_C_SOURCE 200809L

// Inclusion des bibliothèques SDL2 et SDL2_ttf pour la gestion graphique et du texte
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// Bibliothèques standards
#include <stdio.h>    // Pour printf, fprintf, etc.
#include <stdlib.h>   // Pour malloc, free, rand, srand, exit, etc.
#include <stdbool.h>  // Pour le type booléen
#include <string.h>   // Pour strcat, strlen, strdup, memcpy, etc.
#include <ctype.h>    // Pour toupper, tolower
#include <math.h>     // Pour round, etc.
#include <time.h>     // Pour time (initialisation du générateur de nombres aléatoires)

// Définition des dimensions de la fenêtre principale
#define WINDOW_WIDTH      800
#define WINDOW_HEIGHT     900

// Dimensions des zones de jeu dans la fenêtre
#define BOARD_HEIGHT      800                        // Hauteur de la zone du plateau
#undef SCRABBLE_RACK_HEIGHT
#define SCRABBLE_RACK_HEIGHT       50
#define INPUT_AREA_HEIGHT (WINDOW_HEIGHT - BOARD_HEIGHT - SCRABBLE_RACK_HEIGHT) // Hauteur de la zone de saisie

// Marge autour du plateau (en pixels)
#define BOARD_MARGIN      50

// Définition de quelques couleurs utilisées (format RGBA)
static SDL_Color BACKGROUND_COLOR = {255, 255, 255, 255};  // Blanc pour le fond général
static SDL_Color GRID_COLOR       = {255, 255, 255, 255};  // Blanc pour les lignes de grille (peut être modifié)
static SDL_Color TEXT_COLOR       = {0, 0, 0, 255};        // Noir pour le texte
static SDL_Color INPUT_BG_COLOR   = {200, 200, 200, 255};  // Gris clair pour la zone de saisie

// Définition d'un type énuméré pour gérer l'état de saisie utilisateur
typedef enum {
    STATE_IDLE,             // Aucun texte saisi ou aucune action en cours
    STATE_INPUT_TEXT,       // L'utilisateur saisit un mot
    STATE_INPUT_DIRECTION   // L'utilisateur doit saisir la direction (h/v) pour placer un mot de plusieurs lettres
} InputState;

//
// ---------------------- Fonctions pour le Scrabble --------------------------
//

/*
 * Fonction : getLetterScore
 * -------------------------
 * Retourne le score attribué à une lettre (en majuscule) selon la grille de points du Scrabble.
 *
 * Paramètre :
 *   letter : la lettre dont on veut connaître le score.
 *
 * Retour :
 *   Un entier correspondant au score de la lettre.
 */
int getLetterScore(char letter) {
    // Convertit la lettre en majuscule
    letter = toupper(letter);
    // Vérifie dans quelle catégorie se trouve la lettre et retourne le score associé
    if (strchr("AEILNORSTU", letter))
        return 1;
    else if (strchr("DGM", letter))
        return 2;
    else if (strchr("BCP", letter))
        return 3;
    else if (strchr("FHV", letter))
        return 4;
    else if (strchr("JQ", letter))
        return 8;
    else if (strchr("KWXYZ", letter))
        return 10;
    return 0; // Retourne 0 par défaut (si la lettre n'est pas reconnue)
}

/*
 * Fonction : drawRandomLetter
 * -----------------------------
 * Tire une lettre aléatoire selon la distribution classique du Scrabble.
 * La distribution indique combien de fois chaque lettre doit apparaître.
 *
 * Retour :
 *   La lettre aléatoire choisie.
 */
char drawRandomLetter() {
    // Définition de la distribution des lettres (sans jokers)
    struct { char letter; int count; } distribution[] = {
        {'A', 9}, {'B', 2}, {'C', 2}, {'D', 4}, {'E', 12},
        {'F', 2}, {'G', 3}, {'H', 2}, {'I', 9}, {'J', 1},
        {'K', 1}, {'L', 4}, {'M', 2}, {'N', 6}, {'O', 8},
        {'P', 2}, {'Q', 1}, {'R', 6}, {'S', 4}, {'T', 6},
        {'U', 4}, {'V', 2}, {'W', 2}, {'X', 1}, {'Y', 2},
        {'Z', 1}
    };
    // Calcul du nombre total de lettres disponibles
    int total = 0;
    for (int i = 0; i < 26; i++)
        total += distribution[i].count;
    // Tire un nombre aléatoire entre 0 et total - 1
    int r = rand() % total;
    // Parcours la distribution et retourne la lettre correspondante
    for (int i = 0; i < 26; i++) {
        if (r < distribution[i].count)
            return distribution[i].letter;
        r -= distribution[i].count;
    }
    return 'A'; // Valeur par défaut (ne devrait jamais arriver)
}

/*
 * Fonction : canPlaceWord
 * ------------------------
 * Vérifie si un mot peut être placé sur le plateau à partir de la case (startX, startY)
 * dans la direction spécifiée ('h' pour horizontal ou 'v' pour vertical).
 *
 * Le mot peut être placé si :
 *  - Toutes les cases du mot sont dans les limites du plateau.
 *  - Pour chaque lettre du mot :
 *       - Si la case est vide, le rack doit contenir la lettre.
 *       - Si la case n'est pas vide, la lettre existante doit correspondre à celle du mot.
 *  - Si c'est le premier coup (totalPoints == 0), le mot doit passer par la case centrale.
 *  - Si ce n'est pas le premier coup, le mot doit intersecter au moins une lettre déjà présente.
 *
 * Paramètres :
 *   word      : le mot à placer.
 *   startX    : la colonne de départ.
 *   startY    : la ligne de départ.
 *   dir       : la direction ('h' ou 'v').
 *   board     : le plateau actuel (tableau 2D de caractères).
 *   boardSize : la taille du plateau (nombre de colonnes/ligues).
 *   rack      : les lettres disponibles sur le chevalet.
 *   totalPoints : le score total actuel (pour savoir si c'est le premier coup).
 *
 * Retour :
 *   true si le mot peut être placé, false sinon.
 */
bool canPlaceWord(const char *word, int startX, int startY, char dir,
                  char **board, int boardSize, const char *rack, int totalPoints) {
    bool intersects = false;           // Indique si le mot croise une lettre déjà présente
    bool passesThroughCenter = false;  // Indique si le mot passe par la case centrale
    int freq[26] = {0};                // Tableau pour compter les occurrences de chaque lettre dans le rack

    // Remplit le tableau de fréquences avec les lettres du rack (en majuscules)
    for (int i = 0; i < 7; i++) {
        char c = rack[i];
        if (c >= 'A' && c <= 'Z')
            freq[c - 'A']++;
    }

    int len = strlen(word);
    // Parcours chaque lettre du mot
    for (int i = 0; i < len; i++) {
        int x = startX, y = startY;
        // Calcule la position de la lettre selon la direction
        if (dir == 'h')
            x += i;
        else
            y += i;

        // Vérifie que la position reste dans les limites du plateau
        if (x < 0 || x >= boardSize || y < 0 || y >= boardSize)
            return false;

        char boardLetter = board[y][x];         // Lettre déjà présente sur le plateau
        char wordLetter = toupper(word[i]);       // Lettre du mot en majuscule

        if (boardLetter == ' ') {
            // Si la case est vide, vérifie que le rack contient la lettre
            if (freq[wordLetter - 'A'] <= 0)
                return false;
            freq[wordLetter - 'A']--;  // Consomme une occurrence de la lettre
        } else {
            // Si la case n'est pas vide, la lettre doit correspondre
            if (toupper(boardLetter) != wordLetter)
                return false;
            intersects = true;
        }
        // Vérifie si la lettre se trouve sur la case centrale du plateau
        if (x == boardSize / 2 && y == boardSize / 2)
            passesThroughCenter = true;
    }
    // Pour un coup ultérieur, le mot doit croiser au moins une lettre déjà placée
    if (totalPoints > 0 && !intersects)
        return false;
    // Pour le premier coup, le mot doit passer par la case centrale
    if (totalPoints == 0 && !passesThroughCenter)
        return false;
    return true;
}

/*
 * Fonction : placeWord
 * ----------------------
 * Place un mot sur le plateau à partir de la case (startX, startY)
 * dans la direction donnée ('h' ou 'v').
 *
 * Pour chaque case vide sur laquelle le mot doit être placé, la lettre est écrite
 * et la lettre correspondante est remplacée dans le rack par une nouvelle lettre tirée aléatoirement.
 *
 * Paramètres :
 *   word      : le mot à placer.
 *   startX    : la colonne de départ.
 *   startY    : la ligne de départ.
 *   dir       : la direction ('h' ou 'v').
 *   board     : le plateau (tableau 2D de caractères).
 *   rack      : le rack de lettres (tableau de 7 caractères).
 */
void placeWord(const char *word, int startX, int startY, char dir,
               char **board, char *rack) {
    int len = strlen(word);
    // Parcours chaque lettre du mot
    for (int i = 0; i < len; i++) {
        int x = startX, y = startY;
        // Calcule la position de la lettre selon la direction
        if (dir == 'h')
            x += i;
        else
            y += i;
        // Si la case est vide, place la lettre
        if (board[y][x] == ' ') {
            board[y][x] = toupper(word[i]);
            // Consomme la lettre du rack : remplace la lettre utilisée par une lettre aléatoire
            for (int j = 0; j < 7; j++) {
                if (toupper(rack[j]) == toupper(word[i])) {
                    rack[j] = drawRandomLetter();
                    break;
                }
            }
        }
    }
}

//
// --------------------- Fonctions de gestion du dictionnaire ------------------
//

/*
 * Fonction : caseInsensitiveCompare
 * -----------------------------------
 * Fonction de comparaison insensible à la casse utilisée pour trier le dictionnaire.
 *
 * Paramètres :
 *   a, b : pointeurs vers les chaînes à comparer.
 *
 * Retour :
 *   Un entier négatif, nul ou positif selon que a est respectivement
 *   inférieur, égal ou supérieur à b (sans tenir compte de la casse).
 */
int caseInsensitiveCompare(const void *a, const void *b) {
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcasecmp(str1, str2);
}

/*
 * Fonction : loadDictionary
 * --------------------------
 * Charge un dictionnaire depuis un fichier texte (chaque ligne représente un mot).
 * Les mots sont stockés dans un tableau de chaînes qui est ensuite trié de façon insensible à la casse.
 *
 * Paramètres :
 *   filename  : nom du fichier dictionnaire.
 *   outCount  : pointeur vers un entier dans lequel sera stocké le nombre de mots chargés.
 *
 * Retour :
 *   Un tableau de chaînes (char **) contenant les mots, ou NULL en cas d'erreur.
 */
char **loadDictionary(const char *filename, int *outCount) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s\n", filename);
        return NULL;
    }
    int capacity = 100;  // Capacité initiale du tableau
    int count = 0;       // Nombre de mots chargés
    char **words = malloc(capacity * sizeof(char *));
    if (!words) {
        fclose(fp);
        return NULL;
    }
    char buffer[100];
    // Lecture du fichier ligne par ligne
    while (fgets(buffer, sizeof(buffer), fp)) {
        // Supprime le saut de ligne en fin de chaîne
        buffer[strcspn(buffer, "\n")] = '\0';
        // Duplique la chaîne et la stocke dans le tableau
        words[count] = strdup(buffer);
        if (!words[count]) {
            // En cas d'erreur, libère la mémoire allouée
            for (int i = 0; i < count; i++)
                free(words[i]);
            free(words);
            fclose(fp);
            return NULL;
        }
        count++;
        // Si le tableau est plein, on double sa capacité
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
    // Trie les mots de façon insensible à la casse
    qsort(words, count, sizeof(char *), caseInsensitiveCompare);
    *outCount = count;
    return words;
}

/*
 * Fonction : binarySearch
 * ------------------------
 * Effectue une recherche dichotomique dans un tableau trié de chaînes.
 *
 * Paramètres :
 *   array  : le tableau de chaînes trié.
 *   size   : le nombre d'éléments dans le tableau.
 *   target : la chaîne recherchée.
 *
 * Retour :
 *   L'indice de la chaîne dans le tableau si elle est trouvée, -1 sinon.
 */
int binarySearch(char **array, int size, const char *target) {
    int low = 0;
    int high = size - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        int cmp = strcasecmp(array[mid], target);
        if (cmp == 0)
            return mid;
        else if (cmp < 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return -1;
}

/*
 * Fonction : isValidWordBinary
 * ----------------------------
 * Vérifie si un mot existe dans le dictionnaire en effectuant une recherche dichotomique.
 *
 * Paramètres :
 *   word            : le mot à vérifier.
 *   dictionary      : le tableau de mots du dictionnaire.
 *   dictionaryCount : le nombre de mots dans le dictionnaire.
 *
 * Retour :
 *   true si le mot est trouvé, false sinon.
 */
bool isValidWordBinary(const char *word, char **dictionary, int dictionaryCount) {
    return binarySearch(dictionary, dictionaryCount, word) >= 0;
}

/*
 * Fonction : validatePlacement
 * ----------------------------
 * Valide le placement d'un mot en simulant son placement sur une copie temporaire du plateau.
 * Ensuite, pour chaque lettre nouvellement posée, vérifie le mot perpendiculaire (mot croisé).
 *
 * Paramètres :
 *   word            : le mot à placer.
 *   startX, startY  : la position de départ sur le plateau.
 *   dir             : la direction ('h' ou 'v').
 *   board           : le plateau actuel.
 *   boardSize       : la taille du plateau.
 *   dictionary      : le dictionnaire des mots valides.
 *   dictionaryCount : le nombre de mots dans le dictionnaire.
 *
 * Retour :
 *   true si le placement est valide (tous les mots croisés sont valides), false sinon.
 */
bool validatePlacement(const char *word, int startX, int startY, char dir,
                       char **board, int boardSize, char **dictionary, int dictionaryCount) {
    int len = strlen(word);
    bool valid = true;

    // Crée une copie temporaire du plateau pour simuler la pose du mot
    char **tempBoard = malloc(boardSize * sizeof(char *));
    if (!tempBoard)
        return false;
    for (int i = 0; i < boardSize; i++) {
        tempBoard[i] = malloc(boardSize * sizeof(char));
        if (!tempBoard[i]) {
            for (int j = 0; j < i; j++)
                free(tempBoard[j]);
            free(tempBoard);
            return false;
        }
        memcpy(tempBoard[i], board[i], boardSize * sizeof(char));
    }

    // Simule la pose du mot sur la copie temporaire
    for (int i = 0; i < len; i++) {
        int x = startX, y = startY;
        if (dir == 'h')
            x += i;
        else
            y += i;
        // Si la position est hors bornes, le placement est invalide
        if (x < 0 || x >= boardSize || y < 0 || y >= boardSize) {
            valid = false;
            goto cleanup;
        }
        // Si la case est vide, on y écrit la lettre
        if (tempBoard[y][x] == ' ')
            tempBoard[y][x] = toupper(word[i]);
    }

    // Pour chaque lettre du mot placé, on vérifie le mot croisé (perpendiculaire)
    for (int i = 0; i < len; i++) {
        int x = startX, y = startY;
        if (dir == 'h')
            x += i;
        else
            y += i;
        char cross[100];
        int idx = 0;
        if (dir == 'h') {
            // Recherche du mot vertical en colonne x
            int r = y;
            while (r > 0 && tempBoard[r - 1][x] != ' ')
                r--;
            int end = y;
            while (end < boardSize - 1 && tempBoard[end + 1][x] != ' ')
                end++;
            if (end - r + 1 > 1) {
                for (int k = r; k <= end; k++) {
                    cross[idx++] = tempBoard[k][x];
                }
                cross[idx] = '\0';
                if (!isValidWordBinary(cross, dictionary, dictionaryCount)) {
                    valid = false;
                    goto cleanup;
                }
            }
        } else {
            // Recherche du mot horizontal en ligne y
            int c = x;
            while (c > 0 && tempBoard[y][c - 1] != ' ')
                c--;
            int end = x;
            while (end < boardSize - 1 && tempBoard[y][end + 1] != ' ')
                end++;
            if (end - c + 1 > 1) {
                for (int k = c; k <= end; k++) {
                    cross[idx++] = tempBoard[y][k];
                }
                cross[idx] = '\0';
                if (!isValidWordBinary(cross, dictionary, dictionaryCount)) {
                    valid = false;
                    goto cleanup;
                }
            }
        }
    }
cleanup:
    // Libère la mémoire allouée pour la copie temporaire
    for (int i = 0; i < boardSize; i++)
        free(tempBoard[i]);
    free(tempBoard);
    return valid;
}

//
// ---------------------- Fonctions de rendu graphique ------------------------
//

/*
 * Fonction : drawGrid
 * --------------------
 * Dessine la grille du plateau sur l'écran.
 *
 * Paramètres :
 *   renderer         : le renderer SDL utilisé pour dessiner.
 *   boardSize        : la taille du plateau (nombre de cases par ligne/colonne).
 *   boardDrawWidth   : largeur en pixels de la zone de dessin du plateau.
 *   boardDrawHeight  : hauteur en pixels de la zone de dessin du plateau.
 */
void drawGrid(SDL_Renderer *renderer, int boardSize, int boardDrawWidth, int boardDrawHeight) {
    // Efface l'écran avec la couleur de fond
    SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, BACKGROUND_COLOR.a);
    SDL_RenderClear(renderer);
    
    // Calcule la taille d'une case
    float cellWidth = (float)boardDrawWidth / boardSize;
    float cellHeight = (float)boardDrawHeight / boardSize;
    
    // Dessine les lignes verticales
    for (int i = 0; i <= boardSize; i++) {
        int x = BOARD_MARGIN + (int)(i * cellWidth);
        SDL_RenderDrawLine(renderer, x, BOARD_MARGIN, x, BOARD_MARGIN + boardDrawHeight);
    }
    // Dessine les lignes horizontales
    for (int j = 0; j <= boardSize; j++) {
        int y = BOARD_MARGIN + (int)(j * cellHeight);
        SDL_RenderDrawLine(renderer, BOARD_MARGIN, y, BOARD_MARGIN + boardDrawWidth, y);
    }
}

/*
 * Fonction : drawBoard
 * --------------------
 * Dessine le plateau de jeu, y compris les cases, les bonus et les lettres placées.
 *
 * Paramètres :
 *   renderer         : le renderer SDL.
 *   boardFont        : police utilisée pour afficher les lettres.
 *   valueFont        : police utilisée pour afficher la valeur des lettres.
 *   board            : le plateau de jeu (tableau 2D de caractères).
 *   boardSize        : taille du plateau.
 *   boardDrawWidth   : largeur de la zone de dessin du plateau.
 *   boardDrawHeight  : hauteur de la zone de dessin du plateau.
 *   gridThickness    : épaisseur des lignes de la grille.
 */
void drawBoard(SDL_Renderer *renderer, TTF_Font *boardFont, TTF_Font *valueFont,
               char **board, int boardSize, int boardDrawWidth, int boardDrawHeight, int gridThickness) {
    float cellWidth = (float)boardDrawWidth / boardSize;
    float cellHeight = (float)boardDrawHeight / boardSize;
    
    // Définition d'un tableau bonus (standard Scrabble) pour colorer certaines cases
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
    
    // Parcours toutes les cases du plateau
    for (int y = 0; y < boardSize; y++) {
        for (int x = 0; x < boardSize; x++) {
            // Détermine le rectangle correspondant à la case
            SDL_Rect cellRect = {
                BOARD_MARGIN + (int)(x * cellWidth),
                BOARD_MARGIN + (int)(y * cellHeight),
                (int)cellWidth,
                (int)cellHeight
            };
            // Colorie la case en fonction des bonus ou si c'est la case centrale
            if (x == boardSize/2 && y == boardSize/2)
                SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255); // Jaune doré pour la case centrale
            else if (bonusBoard[y][x] == 1)
                SDL_SetRenderDrawColor(renderer, 200, 39, 34, 255);  // Rouge pour certains bonus
            else if (bonusBoard[y][x] == 2)
                SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);  // Orange
            else if (bonusBoard[y][x] == 3)
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);    // Bleu
            else if (bonusBoard[y][x] == 4)
                SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255); // Bleu clair
            else
                SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);   // Vert pour les autres cases
            SDL_RenderFillRect(renderer, &cellRect);
            
            // Si une lettre est placée sur cette case, dessine un overlay beige (pour mettre en évidence la lettre)
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
                SDL_SetRenderDrawColor(renderer, 245, 245, 220, 255); // Beige clair
                SDL_RenderFillRect(renderer, &overlayRect);
            }
        }
    }
    
    // Dessine la grille par-dessus le plateau
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
    
    // Dessine les lettres et leurs valeurs sur chaque case où une lettre est présente
    for (int y = 0; y < boardSize; y++) {
        for (int x = 0; x < boardSize; x++) {
            char letter = board[y][x];
            if (letter != ' ') {
                // Prépare la chaîne contenant la lettre
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
                // Prépare la chaîne affichant la valeur de la lettre
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
}

/*
 * Fonction : drawRack
 * ---------------------
 * Dessine la zone du chevalet (rack) et le bouton "Echanger" dans la zone dédiée en bas de la fenêtre.
 *
 * Paramètres :
 *   renderer       : le renderer SDL.
 *   rackFont       : police utilisée pour afficher les lettres du rack.
 *   valueFont      : police pour afficher la valeur des lettres sur le rack.
 *   rack           : le tableau contenant les lettres du rack.
 *   rackAreaWidth  : largeur de la zone du rack.
 *   SCRABBLE_RACK_HEIGHT    : hauteur de la zone du rack.
 *   startXRack     : position en X de départ pour le rack.
 *   buttonMargin   : marge entre le rack et le bouton.
 *   buttonWidth    : largeur du bouton "Echanger".
 *   buttonHeight   : hauteur du bouton "Echanger".
 *   inputFont      : police utilisée pour le texte du bouton.
 */
void drawRack(SDL_Renderer *renderer, TTF_Font *rackFont, TTF_Font *valueFont,
              char *rack, int rackAreaWidth,
              int startXRack, int buttonMargin, int buttonWidth, int buttonHeight,
              TTF_Font *inputFont) {
    // Dessine le rectangle de fond pour le rack
    SDL_Rect rackRect = { startXRack, BOARD_HEIGHT, rackAreaWidth, SCRABBLE_RACK_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255); // Gris clair
    SDL_RenderFillRect(renderer, &rackRect);
    
    // Pour chaque jeton du rack, dessine une case beige avec la lettre et sa valeur
    for (int i = 0; i < 7; i++) {
        float currentCellWidth = rackAreaWidth / 7.0;
        int tileWidth = (int)round(currentCellWidth * 0.8);
        int tileHeight = (int)round(SCRABBLE_RACK_HEIGHT * 0.8);
        int tileOffsetX = (int)round((currentCellWidth - tileWidth) / 2.0);
        int tileOffsetY = (int)round((SCRABBLE_RACK_HEIGHT - tileHeight) / 2.0);
        int cellX = startXRack + (int)(i * currentCellWidth);
        int cellY = BOARD_HEIGHT;
        SDL_Rect tileRect = { cellX + tileOffsetX, cellY + tileOffsetY, tileWidth, tileHeight };
        SDL_SetRenderDrawColor(renderer, 245, 245, 220, 255); // Beige clair
        SDL_RenderFillRect(renderer, &tileRect);
        // Dessine la lettre du jeton
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
        // Dessine la valeur de la lettre dans le coin inférieur droit du jeton
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
    // Dessine le bouton "Echanger" à côté du rack
    int buttonX = startXRack + rackAreaWidth + buttonMargin;
    int buttonY = BOARD_HEIGHT + (SCRABBLE_RACK_HEIGHT - buttonHeight) / 2;
    SDL_Rect buttonRect = { buttonX, buttonY, buttonWidth, buttonHeight };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Rouge pour le bouton
    SDL_RenderFillRect(renderer, &buttonRect);
    SDL_Surface *btnSurface = TTF_RenderText_Blended(inputFont, "Echanger", TEXT_COLOR);
    if (btnSurface) {
        SDL_Texture *btnTexture = SDL_CreateTextureFromSurface(renderer, btnSurface);
        int btnW, btnH;
        SDL_QueryTexture(btnTexture, NULL, NULL, &btnW, &btnH);
        int btnTextX = buttonX + (buttonWidth - btnW) / 2;
        int btnTextY = buttonY + (buttonHeight - btnH) / 2;
        SDL_Rect btnRect = { btnTextX, btnTextY, btnW, btnH };
        SDL_RenderCopy(renderer, btnTexture, NULL, &btnRect);
        SDL_DestroyTexture(btnTexture);
        SDL_FreeSurface(btnSurface);
    }
}

/*
 * Fonction : drawInputArea
 * --------------------------
 * Dessine la zone de saisie en bas de la fenêtre et affiche le texte de l'invite.
 *
 * Paramètres :
 *   renderer     : le renderer SDL.
 *   inputFont    : police utilisée pour le texte d'invite.
 *   currentState : l'état de saisie actuel (STATE_IDLE, STATE_INPUT_TEXT, STATE_INPUT_DIRECTION).
 *   inputBuffer  : le texte actuellement saisi par l'utilisateur.
 */
void drawInputArea(SDL_Renderer *renderer, TTF_Font *inputFont, InputState currentState, char *inputBuffer) {
    // Définition du rectangle de la zone de saisie
    SDL_Rect inputRect = { 0, BOARD_HEIGHT + SCRABBLE_RACK_HEIGHT, WINDOW_WIDTH, INPUT_AREA_HEIGHT };
    SDL_SetRenderDrawColor(renderer, INPUT_BG_COLOR.r, INPUT_BG_COLOR.g, INPUT_BG_COLOR.b, INPUT_BG_COLOR.a);
    SDL_RenderFillRect(renderer, &inputRect);
    
    // Prépare le texte d'invite en fonction de l'état de saisie
    char displayText[100];
    if (currentState == STATE_IDLE) {
        // Affiche un message indiquant que le premier mot doit passer par la case centrale
        snprintf(displayText, sizeof(displayText), "Entrez un mot (premier mot déjà sur la case centrale)");
    } else if (currentState == STATE_INPUT_TEXT) {
        // Affiche le texte saisi jusqu'à présent
        snprintf(displayText, sizeof(displayText), "Entrez un mot: %s", inputBuffer);
    } else if (currentState == STATE_INPUT_DIRECTION) {
        // Demande la direction de placement
        snprintf(displayText, sizeof(displayText), "Entrez la direction (h/v): ");
    }
    // Rend le texte de l'invite dans la zone de saisie
    SDL_Surface *promptSurface = TTF_RenderText_Blended(inputFont, displayText, TEXT_COLOR);
    if (promptSurface) {
        SDL_Texture *promptTexture = SDL_CreateTextureFromSurface(renderer, promptSurface);
        int textW, textH;
        SDL_QueryTexture(promptTexture, NULL, NULL, &textW, &textH);
        int posX = 10;
        int posY = BOARD_HEIGHT + SCRABBLE_RACK_HEIGHT + (INPUT_AREA_HEIGHT - textH) / 2;
        SDL_Rect dstRect = { posX, posY, textW, textH };
        SDL_RenderCopy(renderer, promptTexture, NULL, &dstRect);
        SDL_DestroyTexture(promptTexture);
        SDL_FreeSurface(promptSurface);
    }
}

//
// ------------------------- Programme principal ------------------------------
//

int main(int argc, char* argv[]) {
    (void)argc; // Ignorer argc et argv pour éviter les avertissements
    (void)argv;

    // Initialisation de la graine pour les nombres aléatoires
    srand(time(NULL));

    // Chargement du dictionnaire depuis le fichier "mots_filtres.txt"
    int dictionaryCount = 0;
    char **dictionary = loadDictionary("mots_filtres.txt", &dictionaryCount);
    if (!dictionary) {
        fprintf(stderr, "Erreur lors du chargement du dictionnaire.\n");
        return EXIT_FAILURE;
    }

    // Taille du plateau (15x15 pour le Scrabble standard)
    int boardSize = 15;
    
    // Allocation dynamique du plateau (tableau 2D de caractères)
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
        // Initialise toutes les cases du plateau avec un espace (indiquant une case vide)
        memset(board[i], ' ', boardSize);
    }

    // Déclaration d'un plateau bonus (pour colorier certaines cases avec des multiplicateurs)
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

    // Variable pour stocker le total des points accumulés
    int totalPoints = 0;

    // Initialisation de SDL pour la gestion de la vidéo
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Erreur SDL_Init: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    // Initialisation de SDL_ttf pour la gestion des polices de caractères
    if (TTF_Init() != 0) {
        fprintf(stderr, "Erreur TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }
    // Création de la fenêtre principale
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
    // Création du renderer pour dessiner dans la fenêtre
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Erreur SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
    // Active le mode de blend pour gérer la transparence
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Chargement des polices (pour le plateau, le rack, la zone de saisie et la valeur des lettres)
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
    rack[7] = '\0';  // Fin de chaîne

    // Variables de gestion de la saisie utilisateur
    InputState currentState = STATE_IDLE;
    char inputBuffer[50] = "";  // Buffer pour le texte saisi
    int inputLength = 0;        // Longueur actuelle du texte saisi
    int selectedCellX = -1, selectedCellY = -1; // Coordonnées de la case sélectionnée
    int lastWordScore = 0;      // Score du dernier mot placé

    // Calcul des dimensions du plateau de jeu (zone de dessin)
    int boardDrawWidth  = WINDOW_WIDTH - 2 * BOARD_MARGIN;
    int boardDrawHeight = BOARD_HEIGHT - 2 * BOARD_MARGIN;
    float cellWidth  = (float)boardDrawWidth / boardSize;
    float cellHeight = (float)boardDrawHeight / boardSize;
    int gridThickness = 2; // Épaisseur des lignes de la grille

    // Configuration de la zone du rack et du bouton "Echanger"
    int rackAreaWidth = 300;
    float rackCellWidth = (float)rackAreaWidth / 7;
    (void)rackCellWidth; // Variable inutilisée dans la suite
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
    int startXRack = (WINDOW_WIDTH - totalRackWidth) / 2;

    // Pour le premier mot, on force la sélection de la case centrale
    selectedCellX = boardSize / 2;
    selectedCellY = boardSize / 2;
    currentState = STATE_INPUT_TEXT;  // Passe en mode saisie texte
    SDL_StartTextInput();             // Démarre la saisie de texte via SDL

    // Boucle principale du programme
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        // Gestion des événements SDL
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = true;
            
            // Si l'état est idle, on attend un clic pour commencer la saisie
            if (currentState == STATE_IDLE) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int mouseX = e.button.x;
                    int mouseY = e.button.y;
                    // Vérifie si le clic se situe dans la zone du plateau
                    if (mouseX >= BOARD_MARGIN && mouseX < (BOARD_MARGIN + boardDrawWidth) &&
                        mouseY >= BOARD_MARGIN && mouseY < (BOARD_MARGIN + boardDrawHeight)) {
                        // Calcule la case cliquée
                        selectedCellX = (mouseX - BOARD_MARGIN) / cellWidth;
                        selectedCellY = (mouseY - BOARD_MARGIN) / cellHeight;
                        currentState = STATE_INPUT_TEXT; // Passe en mode saisie de mot
                        inputBuffer[0] = '\0';
                        inputLength = 0;
                        SDL_StartTextInput();
                    }
                    // Vérifie si le clic se situe dans la zone du rack (pour échanger les jetons)
                    else if (mouseY >= BOARD_HEIGHT && mouseY < (BOARD_HEIGHT + SCRABBLE_RACK_HEIGHT)) {
                        int buttonX = startXRack + rackAreaWidth + buttonMargin;
                        int buttonY = BOARD_HEIGHT + (SCRABBLE_RACK_HEIGHT - buttonHeight) / 2;
                        if (mouseX >= buttonX && mouseX < buttonX + buttonWidth &&
                            mouseY >= buttonY && mouseY < buttonY + buttonHeight) {
                            // Rafraîchit le rack avec 7 nouvelles lettres
                            for (int i = 0; i < 7; i++) {
                                rack[i] = drawRandomLetter();
                            }
                            rack[7] = '\0';
                        }
                    }
                }
            }
            // Si l'état est INPUT_TEXT, on gère la saisie du mot
            else if (currentState == STATE_INPUT_TEXT) {
                if (e.type == SDL_TEXTINPUT) {
                    // Ajoute le texte saisi au buffer, sans dépasser la taille maximale
                    if (inputLength + strlen(e.text.text) < sizeof(inputBuffer) - 1) {
                        strcat(inputBuffer, e.text.text);
                        inputLength = strlen(inputBuffer);
                        // Convertit le texte en majuscules
                        for (size_t i = 0; i < strlen(inputBuffer); i++) {
                            inputBuffer[i] = toupper(inputBuffer[i]);
                        }
                    }
                }
                else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_BACKSPACE && inputLength > 0) {
                        // Supprime le dernier caractère saisi
                        inputBuffer[inputLength - 1] = '\0';
                        inputLength--;
                    }
                    else if (e.key.keysym.sym == SDLK_RETURN) {
                        // L'utilisateur valide la saisie avec la touche Entrée
                        SDL_StopTextInput();
                        if (inputLength == 0) {
                            // Si aucun texte n'a été saisi, revient à l'état idle
                            currentState = STATE_IDLE;
                        }
                        else if (!isValidWordBinary(inputBuffer, dictionary, dictionaryCount)) {
                            // Si le mot n'est pas dans le dictionnaire, affiche une erreur dans la console
                            fprintf(stderr, "Mot invalide: %s\n", inputBuffer);
                            inputBuffer[0] = '\0';
                            inputLength = 0;
                            currentState = STATE_IDLE;
                        }
                        else if (inputLength == 1) {
                            // Pour un mot d'une seule lettre, on suppose l'orientation horizontale par défaut
                            if (canPlaceWord(inputBuffer, selectedCellX, selectedCellY, 'h', board, boardSize, rack, totalPoints)) {
                                // Calcul des multiplicateurs de lettre et de mot en fonction du bonus sur la case
                                int letterMultiplier = 1, wordMultiplier = 1;
                                if (board[selectedCellY][selectedCellX] == ' ') {
                                    int bonus = bonusBoard[selectedCellY][selectedCellX];
                                    switch(bonus) {
                                        case 1: wordMultiplier *= 3; break;
                                        case 2: wordMultiplier *= 2; break;
                                        case 3: letterMultiplier = 3; break;
                                        case 4: letterMultiplier = 2; break;
                                        default: break;
                                    }
                                }
                                int score = getLetterScore(inputBuffer[0]) * letterMultiplier;
                                score *= wordMultiplier;
                                if (validatePlacement(inputBuffer, selectedCellX, selectedCellY, 'h', board, boardSize, dictionary, dictionaryCount)) {
                                    lastWordScore = score;
                                    totalPoints += score;
                                    placeWord(inputBuffer, selectedCellX, selectedCellY, 'h', board, rack);
                                    bonusBoard[selectedCellY][selectedCellX] = 0;
                                } else {
                                    fprintf(stderr, "Placement invalide: un mot croisé n'existe pas\n");
                                }
                            }
                            currentState = STATE_IDLE;
                        }
                        else {
                            // Pour un mot de plusieurs lettres, demande de préciser l'orientation
                            currentState = STATE_INPUT_DIRECTION;
                        }
                    }
                    else if (e.key.keysym.sym == SDLK_ESCAPE) {
                        SDL_StopTextInput();
                        currentState = STATE_IDLE;
                    }
                }
            }
            // Si l'état est INPUT_DIRECTION, l'utilisateur doit saisir 'h' ou 'v' pour l'orientation
            else if (currentState == STATE_INPUT_DIRECTION) {
                if (e.type == SDL_KEYDOWN) {
                    char dir = tolower((char)e.key.keysym.sym);
                    if (dir == 'h' || dir == 'v') {
                        if (canPlaceWord(inputBuffer, selectedCellX, selectedCellY, dir, board, boardSize, rack, totalPoints)) {
                            if (!validatePlacement(inputBuffer, selectedCellX, selectedCellY, dir, board, boardSize, dictionary, dictionaryCount)) {
                                fprintf(stderr, "Placement invalide: un mot croisé n'existe pas\n");
                                currentState = STATE_IDLE;
                            } else {
                                int score = 0;
                                int wordMultiplier = 1;
                                int len = strlen(inputBuffer);
                                for (int i = 0; i < len; i++) {
                                    int x = selectedCellX, y = selectedCellY;
                                    if (dir == 'h')
                                        x += i;
                                    else
                                        y += i;
                                    int letterMultiplier = 1;
                                    if (board[y][x] == ' ') {
                                        int bonus = bonusBoard[y][x];
                                        switch(bonus) {
                                            case 1: wordMultiplier *= 3; break;
                                            case 2: wordMultiplier *= 2; break;
                                            case 3: letterMultiplier = 3; break;
                                            case 4: letterMultiplier = 2; break;
                                            default: break;
                                        }
                                    }
                                    score += getLetterScore(inputBuffer[i]) * letterMultiplier;
                                }
                                score *= wordMultiplier;
                                lastWordScore = score;
                                totalPoints += score;
                                placeWord(inputBuffer, selectedCellX, selectedCellY, dir, board, rack);
                                for (int i = 0; i < strlen(inputBuffer); i++) {
                                    int x = selectedCellX, y = selectedCellY;
                                    if (dir == 'h')
                                        x += i;
                                    else
                                        y += i;
                                    if (bonusBoard[y][x] != 0)
                                        bonusBoard[y][x] = 0;
                                }
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

        // Rendu graphique de toutes les zones

        // 1. Dessine le fond et la grille du plateau
        drawGrid(renderer, boardSize, boardDrawWidth, boardDrawHeight);
        // 2. Dessine le plateau (cases, bonus, lettres et leurs valeurs)
        drawBoard(renderer, boardFont, valueFont, board, boardSize, boardDrawWidth, boardDrawHeight, gridThickness);
        // 3. Dessine le rack (chevalet) et le bouton "Echanger"
        drawRack(renderer, rackFont, valueFont, rack, rackAreaWidth, startXRack, buttonMargin, buttonWidth, buttonHeight, inputFont);
        // 4. Dessine la zone de saisie en bas de la fenêtre
        drawInputArea(renderer, inputFont, currentState, inputBuffer);
        
        // 5. Affiche le score du dernier mot dans le coin supérieur droit
        {
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
        }
        // 6. Affiche le total des points dans le coin supérieur gauche
        {
            char totalText[50];
            snprintf(totalText, sizeof(totalText), "Total: %d", totalPoints);
            SDL_Surface *totalSurface = TTF_RenderText_Blended(boardFont, totalText, TEXT_COLOR);
            if (totalSurface) {
                SDL_Texture *totalTexture = SDL_CreateTextureFromSurface(renderer, totalSurface);
                int totalW, totalH;
                SDL_QueryTexture(totalTexture, NULL, NULL, &totalW, &totalH);
                SDL_Rect totalRect = { 10, 10, totalW, totalH };
                SDL_RenderCopy(renderer, totalTexture, NULL, &totalRect);
                SDL_DestroyTexture(totalTexture);
                SDL_FreeSurface(totalSurface);
            }
        }
        // 7. (Optionnel) Redessine les lettres sur le plateau pour s'assurer qu'elles soient bien visibles
        // Cette partie est redondante avec drawBoard et peut être omise si désiré.
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
        
        // Met à jour l'écran avec tout le rendu réalisé
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // Pause pour limiter la fréquence de rafraîchissement (~60 FPS)
    }

    // Libération de la mémoire allouée pour le plateau
    for (int i = 0; i < boardSize; i++)
        free(board[i]);
    free(board);

    // Libération de la mémoire allouée pour le dictionnaire
    for (int i = 0; i < dictionaryCount; i++) {
        free(dictionary[i]);
    }
    free(dictionary);

    // Fermeture et libération des polices
    TTF_CloseFont(valueFont);
    TTF_CloseFont(inputFont);
    TTF_CloseFont(rackFont);
    TTF_CloseFont(boardFont);

    // Destruction du renderer et de la fenêtre, puis arrêt de SDL_ttf et SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}


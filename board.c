#include "board.h"
#include "dictionary.h"

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

    // Utilisation de switch case pour une bonne lisibilité et performance
    switch (letter) {
        case 'A': case 'E': case 'I': case 'L': case 'N':
        case 'O': case 'R': case 'S': case 'T': case 'U':
            return 1;

        case 'D': case 'G': case 'M':
            return 2;

        case 'B': case 'C': case 'P':
            return 3;

        case 'F': case 'H': case 'V':
            return 4;

        case 'J': case 'Q':
            return 8;

        case 'K': case 'W': case 'X': case 'Y': case 'Z':
            return 10;

        default:
            return 0; // Retourne 0 si la lettre n'est pas valide
    }
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
bool intersects = false;           // Indique si le mot croise (ou touche) une lettre déjà présente
bool passesThroughCenter = false;  // Indique si le mot passe par la case centrale
int freq[26] = {0};                // Occurrences des lettres disponibles sur le rack

// Remplit le tableau de fréquences avec les lettres du rack (en majuscules)
for (int i = 0; i < 7; i++) {
char c = rack[i];
if (c >= 'A' && c <= 'Z')
freq[c - 'A']++;
}

int len = strlen(word);
bool newLetterPlaced = false;      // Nouvel indicateur : au moins une lettre doit être placée dans une case vide

// Vérifie la case immédiatement avant le début du mot (si elle existe)
if (dir == 'h') {
if (startX > 0 && board[startY][startX - 1] != ' ')
return false;
} else { // placement vertical
if (startY > 0 && board[startY - 1][startX] != ' ')
return false;
}

// Parcourt chaque lettre du mot à placer
for (int i = 0; i < len; i++) {
int x = startX, y = startY;
// Calcule la position de la lettre selon la direction (horizontale ou verticale)
if (dir == 'h')
x += i;
else
y += i;

// Vérifie que la position reste dans les limites du plateau
if (x < 0 || x >= boardSize || y < 0 || y >= boardSize)
return false;

char boardLetter = board[y][x];         // Lettre déjà présente sur le plateau (si la case n'est pas vide)
char wordLetter = toupper(word[i]);       // Lettre du mot (convertie en majuscule)

if (boardLetter == ' ') {
// Si la case est vide, il faut que le rack contienne la lettre
if (freq[wordLetter - 'A'] <= 0)
  return false;
freq[wordLetter - 'A']--;  // Consomme une occurrence de la lettre du rack
newLetterPlaced = true;    // Marque qu'une nouvelle lettre sera placée

// Vérifie les connexions perpendiculaires pour détecter un contact avec d'autres mots
if (dir == 'h') {
  // Pour une pose horizontale, vérifie au-dessus et en-dessous
  if ((y > 0 && board[y - 1][x] != ' ') ||
      (y < boardSize - 1 && board[y + 1][x] != ' '))
      intersects = true;
} else { // placement vertical
  // Pour une pose verticale, vérifie à gauche et à droite
  if ((x > 0 && board[y][x - 1] != ' ') ||
      (x < boardSize - 1 && board[y][x + 1] != ' '))
      intersects = true;
}
} else {
// Si la case n'est pas vide, la lettre existante doit correspondre à celle du mot
if (toupper(boardLetter) != wordLetter)
  return false;
intersects = true;
}
// Vérifie si la case courante est la case centrale
if (x == boardSize / 2 && y == boardSize / 2)
passesThroughCenter = true;
}

// Vérifie la case immédiatement après la fin du mot (si elle existe)
if (dir == 'h') {
int endX = startX + len;
if (endX < boardSize && board[startY][endX] != ' ')
return false;
} else { // placement vertical
int endY = startY + len;
if (endY < boardSize && board[endY][startX] != ' ')
return false;
}

// Nouveau contrôle : il faut qu'au moins une lettre nouvelle soit placée (case vide utilisée)
if (!newLetterPlaced)
return false;

// Pour un coup ultérieur, le mot doit toucher au moins une lettre déjà présente
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
    char **board, int boardSize, DictionaryEntry *dictionary) {
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
if (!isValidWordHash(cross, dictionary)) {
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
if (!isValidWordHash(cross, dictionary)) {
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





int recalcTotalScore(char **board, int boardSize) {
int total = 0;

// Parcours horizontal (ligne par ligne)
for (int i = 0; i < boardSize; i++) {
int j = 0;
while (j < boardSize) {
// Ignore les cases vides
while (j < boardSize && board[i][j] == ' ') {
j++;
}
int start = j;
int wordScore = 0;
// Construit le mot horizontal en cours
while (j < boardSize && board[i][j] != ' ') {
wordScore += getLetterScore(board[i][j]);
j++;
}
// Si le mot comporte au moins 2 lettres, on l’ajoute
if (j - start > 1) {
total += wordScore;
}
}
}

// Parcours vertical (colonne par colonne)
for (int j = 0; j < boardSize; j++) {
int i = 0;
while (i < boardSize) {
// Ignore les cases vides
while (i < boardSize && board[i][j] == ' ') {
i++;
}
int start = i;
int wordScore = 0;
// Construit le mot vertical en cours
while (i < boardSize && board[i][j] != ' ') {
wordScore += getLetterScore(board[i][j]);
i++;
}
// Si le mot comporte au moins 2 lettres, on l’ajoute
if (i - start > 1) {
total += wordScore;
}
}
}

return total;
}







// =======================================
// Fonction : findBestMove
// =======================================
void findBestMove(char **board, int boardSize,
    DictionaryEntry *dictionary,
    char *rack, 
    int *totalPoints,
    int bonusBoard[15][15])
        {
        int bestScore = 0;
        char bestWord[100] = "";
        int bestX = -1, bestY = -1;
        char bestDir = 'h';

        // Parcours du dictionnaire via HASH_ITER
        DictionaryEntry *entry, *tmp;
        HASH_ITER(hh, dictionary, entry, tmp) {
        const char *word = entry->word;
        int len = strlen(word);
        if (len > boardSize) continue;

        // Parcours de chaque case du plateau
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                
            // Test sur les deux orientations : horizontal ('h') et vertical ('v')
                for (int d = 0; d < 2; d++) {
                char dir = (d == 0) ? 'h' : 'v';

                if (canPlaceWord(word, x, y, dir, board, boardSize, rack, *totalPoints)) {
                    if (validatePlacement(word, x, y, dir, board, boardSize, dictionary)) {
                    int currentScore = 0;
                    int wordMultiplier = 1;
                    for (int i = 0; i < len; i++) {
                        int xx = x, yy = y;
                        if (dir == 'h') {
                        xx += i;
                        } else {
                        yy += i;
                        }
                        if (board[yy][xx] == ' ') {
                        int bonus = bonusBoard[yy][xx];
                        int letterMult = 1;
                        switch (bonus) {
                            case 1: // triple-mot
                                wordMultiplier *= 3;
                                break;
                            case 2: // double-mot
                                wordMultiplier *= 2;
                                break;
                            case 3: // triple-lettre
                                letterMult = 3;
                                break;
                            case 4: // double-lettre
                                letterMult = 2;
                                break;
                            default:
                                break;
                        }
                        currentScore += getLetterScore(toupper(word[i])) * letterMult;
                        } else {
                        currentScore += getLetterScore(board[yy][xx]);
                        }
                        }
                    currentScore *= wordMultiplier;
                    if (currentScore > bestScore) {
                    bestScore = currentScore;
                    strcpy(bestWord, word);
                    bestX = x;
                    bestY = y;
                    bestDir = dir;
                    }
                    }
                    }
                }
                }
            }
        }

        if (bestScore > 0) {
        placeWord(bestWord, bestX, bestY, bestDir, board, rack);
        // Désactivation des bonus pour les cases utilisées
        int len = strlen(bestWord);
        for (int i = 0; i < len; i++) {
        int xx = bestX, yy = bestY;
        if (bestDir == 'h') xx += i;
        else yy += i;
        bonusBoard[yy][xx] = 0;
        }
        // Mettre à jour le score total (choix : recalcul complet)
        *totalPoints += bestScore;
        printf("[Indice] Meilleur coup : %s (%c) en (%d, %d) -> %d points\nRappel pas de bonus 50pts si on fait un scrabble en utilisant l'indice\n",
        bestWord, bestDir, bestX, bestY, bestScore);
        } else {
        printf("[Indice] Aucun coup optimal trouvé...\n");
        }
        }

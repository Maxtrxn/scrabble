#include "board.h"
#include "dictionary.h"

/*
 * Fonction : findBestMove
 * ------------------------
 * Recherche le meilleur coup possible à jouer avec les lettres disponibles sur le chevalet,
 * en parcourant le dictionnaire et en testant toutes les positions valides sur le plateau.
 *
 * Paramètres :
 *   board       : le plateau de jeu (tableau de caractères).
 *   boardSize   : la taille du plateau (généralement 15x15).
 *   dictionary  : table de hachage contenant tous les mots valides.
 *   rack        : lettres disponibles sur le chevalet du joueur.
 *   totalPoints : pointeur vers le score total du joueur (sera mis à jour si un mot est placé).
 *   bonusBoard  : tableau des bonus de points du plateau (double-mot, triple-lettre, etc.).
 *
 * Comportement :
 *   - Parcours du dictionnaire pour tester chaque mot.
 *   - Vérification de toutes les positions du plateau pour placer le mot.
 *   - Évaluation du score pour chaque coup possible en appliquant les bonus.
 *   - Sélection du meilleur coup trouvé (le plus haut score possible).
 *   - Placement du mot si un coup valide est trouvé, mise à jour du plateau et du score.
 *   - Désactivation des bonus pour les cases utilisées.
 *
 * Remarques :
 *   - Cette fonction ne prend pas en compte les échanges de lettres ou les options avancées.
 *   - Elle ne donne pas de bonus de 50 points pour un Scrabble (pose de toutes les lettres du rack).
 *   - Si aucun coup n'est trouvé, elle affiche un message d'erreur.
 */
void findBestMove(char **board, int boardSize,
    DictionaryEntry *dictionary,
    char *rack,
    int *totalPoints,
    int bonusBoard[15][15])
{
    int bestScore = 0;        // Meilleur score trouvé
    char bestWord[100] = "";  // Mot correspondant au meilleur coup
    int bestX = -1, bestY = -1; // Position du mot sur le plateau
    char bestDir = 'h';       // Direction du mot ('h' pour horizontal, 'v' pour vertical)

    // Parcours du dictionnaire via HASH_ITER (balayage de la table de hachage)
    DictionaryEntry *entry, *tmp;
    HASH_ITER(hh, dictionary, entry, tmp) {
        const char *word = entry->word;
        int len = strlen(word);
        if (len > boardSize) continue; // Ignore les mots trop longs pour le plateau

        // Parcours chaque case du plateau
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                
                // Teste les deux orientations : horizontal ('h') et vertical ('v')
                for (int d = 0; d < 2; d++) {
                    char dir = (d == 0) ? 'h' : 'v';

                    // Vérifie si le mot peut être placé à cette position
                    if (canPlaceWord(word, x, y, dir, board, boardSize, rack, *totalPoints)) {
                        // Vérifie si les mots croisés générés sont valides
                        if (validatePlacement(word, x, y, dir, board, boardSize, dictionary)) {
                            int currentScore = 0;  // Score du mot testé
                            int wordMultiplier = 1; // Multiplicateur pour les bonus mots

                            // Calcul du score du mot en prenant en compte les bonus
                            for (int i = 0; i < len; i++) {
                                int xx = x, yy = y;
                                if (dir == 'h') {
                                    xx += i;
                                } else {
                                    yy += i;
                                }

                                // Vérifie si la case est vide (lettre du rack placée)
                                if (board[yy][xx] == ' ') {
                                    int bonus = bonusBoard[yy][xx];
                                    int letterMult = 1;
                                    switch (bonus) {
                                        case 1: // Triple-mot
                                            wordMultiplier *= 3;
                                            break;
                                        case 2: // Double-mot
                                            wordMultiplier *= 2;
                                            break;
                                        case 3: // Triple-lettre
                                            letterMult = 3;
                                            break;
                                        case 4: // Double-lettre
                                            letterMult = 2;
                                            break;
                                        default:
                                            break;
                                    }
                                    // Applique le multiplicateur de lettre
                                    currentScore += getLetterScore(toupper(word[i])) * letterMult;
                                } else {
                                    // Ajoute directement la valeur de la lettre existante sur le plateau
                                    currentScore += getLetterScore(board[yy][xx]);
                                }
                            }

                            // Applique le multiplicateur de mot final
                            currentScore *= wordMultiplier;

                            // Vérifie si ce coup est meilleur que le précédent
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

    // Si un coup optimal a été trouvé, le placer sur le plateau
    if (bestScore > 0) {
        placeWord(bestWord, bestX, bestY, bestDir, board, rack);

        // Désactive les bonus sur les cases utilisées
        int len = strlen(bestWord);
        for (int i = 0; i < len; i++) {
            int xx = bestX, yy = bestY;
            if (bestDir == 'h') xx += i;
            else yy += i;
            bonusBoard[yy][xx] = 0; // Efface les bonus après utilisation
        }

        // Mettre à jour le score total
        *totalPoints += bestScore;

        // Affichage du coup joué par l'IA
        printf("[Indice] Meilleur coup : %s (%c) en (%d, %d) -> %d points\n"
               "Rappel : pas de bonus 50pts si on fait un scrabble en utilisant l'indice\n",
               bestWord, bestDir, bestX, bestY, bestScore);
    } else {
        // Aucun coup trouvé
        printf("[Indice] Aucun coup optimal trouvé...\n");
    }
}
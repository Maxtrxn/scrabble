#include "dictionary.h"

/*
 * Fonction : loadDictionaryHash
 * ------------------------------
 * Charge un dictionnaire de mots dans une table de hachage (via uthash).
 * Chaque mot du fichier est stocké dans une structure DictionaryEntry et inséré dans la table.
 *
 * Paramètres :
 *   filename : chemin du fichier contenant la liste des mots du dictionnaire.
 *
 * Retour :
 *   Un pointeur vers la table de hachage contenant les mots du dictionnaire.
 *
 * Remarque :
 *   - En cas d'échec d'ouverture du fichier, le programme affiche une erreur et quitte.
 *   - En cas d'échec d'allocation mémoire, le programme affiche une erreur et quitte.
 *   - Chaque mot est inséré en tant qu'entrée unique dans la table de hachage.
 */
DictionaryEntry* loadDictionaryHash(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s\n", filename);
        exit(EXIT_FAILURE);
    }

    DictionaryEntry *dictionary = NULL;  // Table de hachage initialement vide
    char buffer[100];  // Buffer temporaire pour stocker chaque mot lu du fichier

    // Lecture du fichier ligne par ligne
    while (fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\n")] = '\0'; // Supprime le saut de ligne

        // Alloue une nouvelle entrée pour le mot
        DictionaryEntry *entry = malloc(sizeof(DictionaryEntry));
        if (!entry) {
            fprintf(stderr, "Erreur d'allocation mémoire.\n");
            exit(EXIT_FAILURE);
        }

        // Copie le mot lu dans la structure et s'assure de la terminaison correcte
        strncpy(entry->word, buffer, sizeof(entry->word));
        entry->word[sizeof(entry->word) - 1] = '\0';

        // Ajoute l'entrée à la table de hachage
        HASH_ADD_STR(dictionary, word, entry);
    }

    fclose(fp);
    return dictionary;  // Retourne le dictionnaire chargé en mémoire sous forme de table de hachage
}


/*
 * Fonction : isValidWordHash
 * --------------------------
 * Vérifie si un mot existe dans la table de hachage du dictionnaire.
 *
 * Paramètres :
 *   word       : le mot à vérifier.
 *   dictionary : la table de hachage contenant les mots du dictionnaire.
 *
 * Retour :
 *   true si le mot est trouvé dans le dictionnaire, false sinon.
 *
 * Remarque :
 *   - Utilise la fonction HASH_FIND_STR d'uthash pour rechercher directement dans la table.
 *   - La recherche est optimisée grâce à la structure de hachage, ce qui la rend très rapide.
 */
bool isValidWordHash(const char *word, DictionaryEntry *dictionary) {
    DictionaryEntry *entry;
    HASH_FIND_STR(dictionary, word, entry);  // Recherche du mot dans la table de hachage
    return entry != NULL;  // Retourne vrai si trouvé, faux sinon
}


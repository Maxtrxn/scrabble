#include "dictionary.h"

DictionaryEntry* loadDictionaryHash(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s\n", filename);
        exit(EXIT_FAILURE);
    }
    DictionaryEntry *dictionary = NULL;
    char buffer[100];
    while (fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        DictionaryEntry *entry = malloc(sizeof(DictionaryEntry));
        if (!entry) {
            fprintf(stderr, "Erreur d'allocation mémoire.\n");
            exit(EXIT_FAILURE);
        }
        strncpy(entry->word, buffer, sizeof(entry->word));
        entry->word[sizeof(entry->word) - 1] = '\0';
        HASH_ADD_STR(dictionary, word, entry);
    }
    fclose(fp);
    return dictionary;
}

bool isValidWordHash(const char *word, DictionaryEntry *dictionary) {
    DictionaryEntry *entry;
    HASH_FIND_STR(dictionary, word, entry);
    return entry != NULL;
}

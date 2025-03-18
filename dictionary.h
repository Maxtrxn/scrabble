#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "scrabble.h"

// Charge le dictionnaire depuis un fichier et le place dans une table de hachage.
DictionaryEntry* loadDictionaryHash(const char *filename);

// Vérifie si un mot est présent dans le dictionnaire.
bool isValidWordHash(const char *word, DictionaryEntry *dictionary);

#endif  // DICTIONARY_H

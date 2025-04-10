#include "board.h"
#include "dictionary.h"

void findBestMove(char **board, int boardSize,
    DictionaryEntry *dictionary,
    char *rack,
    int *totalPoints,
    int bonusBoard[15][15]);
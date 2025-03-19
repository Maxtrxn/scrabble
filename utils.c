#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// Fonction d'initialisation de SDL, TTF, création de la fenêtre, du renderer et chargement des polices
int initResources(Resources *res) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Erreur SDL_Init: %s\n", SDL_GetError());
        return -1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "Erreur TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return -1;
    }
    res->window = SDL_CreateWindow("Scrabble",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   WINDOW_WIDTH, WINDOW_HEIGHT,
                                   SDL_WINDOW_SHOWN);
    if (!res->window) {
        fprintf(stderr, "Erreur SDL_CreateWindow: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    res->renderer = SDL_CreateRenderer(res->window, -1, SDL_RENDERER_ACCELERATED);
    if (!res->renderer) {
        fprintf(stderr, "Erreur SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(res->window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    SDL_SetRenderDrawBlendMode(res->renderer, SDL_BLENDMODE_BLEND);
    
    res->boardFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 28);
    if (!res->boardFont) {
        fprintf(stderr, "Erreur TTF_OpenFont (boardFont): %s\n", TTF_GetError());
        SDL_DestroyRenderer(res->renderer);
        SDL_DestroyWindow(res->window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    res->rackFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20);
    if (!res->rackFont) {
        fprintf(stderr, "Erreur TTF_OpenFont (rackFont): %s\n", TTF_GetError());
        TTF_CloseFont(res->boardFont);
        SDL_DestroyRenderer(res->renderer);
        SDL_DestroyWindow(res->window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    res->inputFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
    if (!res->inputFont) {
        fprintf(stderr, "Erreur TTF_OpenFont (inputFont): %s\n", TTF_GetError());
        TTF_CloseFont(res->rackFont);
        TTF_CloseFont(res->boardFont);
        SDL_DestroyRenderer(res->renderer);
        SDL_DestroyWindow(res->window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    res->valueFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12);
    if (!res->valueFont) {
        fprintf(stderr, "Erreur TTF_OpenFont (valueFont): %s\n", TTF_GetError());
        TTF_CloseFont(res->inputFont);
        TTF_CloseFont(res->rackFont);
        TTF_CloseFont(res->boardFont);
        SDL_DestroyRenderer(res->renderer);
        SDL_DestroyWindow(res->window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    return 0;
}

// Alloue et initialise le plateau avec des espaces
char **initBoard(int boardSize) {
    char **board = malloc(boardSize * sizeof(char *));
    if (!board) {
        fprintf(stderr, "Erreur d'allocation mémoire pour le plateau.\n");
        return NULL;
    }
    for (int i = 0; i < boardSize; i++) {
        board[i] = malloc(boardSize * sizeof(char));
        if (!board[i]) {
            fprintf(stderr, "Erreur d'allocation mémoire.\n");
            for (int j = 0; j < i; j++)
                free(board[j]);
            free(board);
            return NULL;
        }
        memset(board[i], ' ', boardSize);
    }
    return board;
}

void freeBoard(char **board, int boardSize) {
    for (int i = 0; i < boardSize; i++)
        free(board[i]);
    free(board);
}

// Libère toutes les ressources allouées
void cleanup(Resources *res, DictionaryEntry *dictionaryHash, char **board, int boardSize) {
    DictionaryEntry *current, *tmp;
    HASH_ITER(hh, dictionaryHash, current, tmp) {
        HASH_DEL(dictionaryHash, current);
        free(current);
    }
    freeBoard(board, boardSize);
    TTF_CloseFont(res->valueFont);
    TTF_CloseFont(res->inputFont);
    TTF_CloseFont(res->rackFont);
    TTF_CloseFont(res->boardFont);
    SDL_DestroyRenderer(res->renderer);
    SDL_DestroyWindow(res->window);
    TTF_Quit();
    SDL_Quit();
}
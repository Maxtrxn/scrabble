#include "graphics.h"

// Définition des couleurs (initialisation des variables globales)
SDL_Color BACKGROUND_COLOR = {255, 255, 255, 255};
SDL_Color GRID_COLOR       = {255, 255, 255, 255};
SDL_Color TEXT_COLOR       = {80, 80, 80, 255};
SDL_Color INPUT_BG_COLOR   = {200, 200, 200, 255};

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
                SDL_Surface *textSurface = TTF_RenderUTF8_Blended(boardFont, text, TEXT_COLOR);
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
                SDL_Surface *valueSurface = TTF_RenderUTF8_Blended(valueFont, valueText, TEXT_COLOR);
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
 *   startXRack     : position en X de départ pour le rack.py
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
        SDL_Surface *rackSurface = TTF_RenderUTF8_Blended(rackFont, letter, TEXT_COLOR);
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
        SDL_Surface *valueSurface = TTF_RenderUTF8_Blended(valueFont, valueText, TEXT_COLOR);
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
    SDL_Surface *btnSurface = TTF_RenderUTF8_Blended(inputFont, "Echanger", TEXT_COLOR);
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

    // === Ajout du bouton "Meilleur Coup" ===
    int bestMoveButtonX = buttonX + buttonWidth + 10; // 10px d'écart à droite de "Echanger"
    int bestMoveButtonY = buttonY;
    int bestMoveButtonWidth = 120;  // Largeur fixe (modifiable)
    int bestMoveButtonHeight = buttonHeight; // Même hauteur que "Echanger"
    SDL_Rect bestMoveRect = { bestMoveButtonX, bestMoveButtonY, bestMoveButtonWidth, bestMoveButtonHeight };
    SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255); // Vert
    SDL_RenderFillRect(renderer, &bestMoveRect);

    SDL_Surface *bestSurface = TTF_RenderUTF8_Blended(inputFont, "Indice", TEXT_COLOR);
    if (bestSurface) {
        SDL_Texture *bestTexture = SDL_CreateTextureFromSurface(renderer, bestSurface);
        int bmW, bmH;
        SDL_QueryTexture(bestTexture, NULL, NULL, &bmW, &bmH);
        int bmTextX = bestMoveButtonX + (bestMoveButtonWidth - bmW) / 2;
        int bmTextY = bestMoveButtonY + (bestMoveButtonHeight - bmH) / 2;
        SDL_Rect bmRect = { bmTextX, bmTextY, bmW, bmH };
        SDL_RenderCopy(renderer, bestTexture, NULL, &bmRect);
        SDL_DestroyTexture(bestTexture);
        SDL_FreeSurface(bestSurface);
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
void drawInputArea(SDL_Renderer *renderer, TTF_Font *inputFont, InputState currentState, char *inputBuffer, int totalPoints) {
  SDL_Rect inputRect = { 0, BOARD_HEIGHT + SCRABBLE_RACK_HEIGHT, WINDOW_WIDTH, INPUT_AREA_HEIGHT };
  SDL_SetRenderDrawColor(renderer, INPUT_BG_COLOR.r, INPUT_BG_COLOR.g, INPUT_BG_COLOR.b, INPUT_BG_COLOR.a);
  SDL_RenderFillRect(renderer, &inputRect);
    char displayText[100];
    if (currentState == STATE_IDLE) {
        if (totalPoints == 0)
            snprintf(displayText, sizeof(displayText), "Cliquez pour choisir une case (1er mot doit passer par le milieu)");
        else
            snprintf(displayText, sizeof(displayText), "Cliquez pour choisir une case");
    } else if (currentState == STATE_INPUT_TEXT) {
        snprintf(displayText, sizeof(displayText), "Entrez un mot: %s", inputBuffer);
    } else if (currentState == STATE_INPUT_DIRECTION) {
        snprintf(displayText, sizeof(displayText), "Entrez la direction du mot (h/v): ");
    }
    // Rend le texte de l'invite dans la zone de saisie
    SDL_Surface *promptSurface = TTF_RenderUTF8_Blended(inputFont, displayText, TEXT_COLOR);
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
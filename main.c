#include "scrabble.h"
#include "dictionary.h"
#include "board.h"
#include "graphics.h"


//
// ------------------------- Programme principal ------------------------------
//

int main(int argc, char* argv[]) {
    (void)argc; // Ignorer argc et argv pour éviter les avertissements
    (void)argv;

    // Initialisation de la graine pour les nombres aléatoires
    srand(time(NULL));

    // Chargement du dictionnaire depuis le fichier "mots_filtres.txt"
    DictionaryEntry *dictionaryHash = loadDictionaryHash("mots_filtres.txt");
    if (!dictionaryHash) {
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
    SDL_Surface *btnSurface = TTF_RenderUTF8_Blended(inputFont, "Echanger", TEXT_COLOR);
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
    //selectedCellX = boardSize / 2;
    //selectedCellY = boardSize / 2;
    //currentState = STATE_INPUT_TEXT;  // Passe en mode saisie texte
    //SDL_StartTextInput();             // Démarre la saisie de texte via SDL

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
                    // Sinon, vérifie si le clic se situe dans la zone du rack
                    else if (mouseY >= BOARD_HEIGHT && mouseY < (BOARD_HEIGHT + SCRABBLE_RACK_HEIGHT)) {
                        // Coordonnées et dimensions du bouton "Echanger"
                        int buttonX = startXRack + rackAreaWidth + buttonMargin;
                        int buttonY = BOARD_HEIGHT + (SCRABBLE_RACK_HEIGHT - buttonHeight) / 2;
                        // Si le clic est sur le bouton "Echanger"
                        if (mouseX >= buttonX && mouseX < buttonX + buttonWidth &&
                            mouseY >= buttonY && mouseY < buttonY + buttonHeight) {
                            // Rafraîchit le rack avec 7 nouvelles lettres
                            for (int i = 0; i < 7; i++) {
                                rack[i] = drawRandomLetter();
                            }
                            rack[7] = '\0';
                        }
                        // Vérification du clic sur le bouton "Meilleur Coup"
                        int bestMoveButtonX = buttonX + buttonWidth + 10; // Bouton "Meilleur Coup" placé à droite de "Echanger" avec un décalage de 10 px
                        int bestMoveButtonY = buttonY;
                        int bestMoveButtonWidth = 120;  // Doit correspondre à la largeur définie dans drawRack()
                        int bestMoveButtonHeight = buttonHeight;
                        if (mouseX >= bestMoveButtonX && mouseX < bestMoveButtonX + bestMoveButtonWidth &&
                            mouseY >= bestMoveButtonY && mouseY < bestMoveButtonY + bestMoveButtonHeight) {
                            // Appel de la fonction qui trouve et place le meilleur coup
                            findBestMove(board, boardSize, dictionaryHash, rack, &totalPoints, bonusBoard);
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
                        else if (!isValidWordHash(inputBuffer, dictionaryHash)) {
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
                                if (validatePlacement(inputBuffer, selectedCellX, selectedCellY, 'h', board, boardSize, dictionaryHash)) {
                                  lastWordScore = score;
                                  placeWord(inputBuffer, selectedCellX, selectedCellY, 'h', board, rack);
                                  bonusBoard[selectedCellY][selectedCellX] = 0;
                                  totalPoints = recalcTotalScore(board, boardSize);
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
                            if (!validatePlacement(inputBuffer, selectedCellX, selectedCellY, dir, board, boardSize, dictionaryHash)) {
                                fprintf(stderr, "Placement invalide: un mot croisé n'existe pas\n");
                                currentState = STATE_IDLE;
                            } else {
                                int score = 0;
                                int wordMultiplier = 1;
                                int len = strlen(inputBuffer);
                                int tilesUsed = 0; // Compteur pour le nombre de lettres nouvelles placées
                                for (int i = 0; i < len; i++) {
                                    int x = selectedCellX, y = selectedCellY;
                                    if (dir == 'h')
                                        x += i;
                                    else
                                        y += i;
                                    int letterMultiplier = 1;
                                    if (board[y][x] == ' ') {
                                        tilesUsed++;  // Incrémente le compteur si la case est vide (lettre tirée du rack)
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
                                //printf("tilesUsed = %d\n", tilesUsed);
                                // Si toutes les 7 lettres du rack sont utilisées dans ce coup, ajoute 50 points bonus (Scrabble)
                                if (tilesUsed == 7) {
                                    score += 50;
                                }

                                lastWordScore = score;
                                totalPoints += score;
                                placeWord(inputBuffer, selectedCellX, selectedCellY, dir, board, rack);
                                for (int i = 0; i < (int)strlen(inputBuffer); i++) {
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
        drawInputArea(renderer, inputFont, currentState, inputBuffer, totalPoints);

        
        // 5. Affiche le score du dernier mot dans le coin supérieur droit
        {
            char scoreText[50];
            snprintf(scoreText, sizeof(scoreText), "Points: %d", lastWordScore);
            SDL_Surface *scoreSurface = TTF_RenderUTF8_Blended(boardFont, scoreText, TEXT_COLOR);
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
            SDL_Surface *totalSurface = TTF_RenderUTF8_Blended(boardFont, totalText, TEXT_COLOR);
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
        
        // Met à jour l'écran avec tout le rendu réalisé
        SDL_RenderPresent(renderer);
        SDL_Delay(64); // Pause pour limiter la fréquence de rafraîchissement (~60 FPS)
    }

    // Libération de la mémoire allouée pour le plateau
    for (int i = 0; i < boardSize; i++)
        free(board[i]);
    free(board);

    // Libération de la mémoire allouée pour le dictionnaire (table de hachage)
    DictionaryEntry *current, *tmp;
    HASH_ITER(hh, dictionaryHash, current, tmp) {
        HASH_DEL(dictionaryHash, current);  // Supprime l'entrée de la table
        free(current);  // Libère la mémoire allouée
    }


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
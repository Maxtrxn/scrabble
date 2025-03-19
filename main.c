#include "scrabble.h"
#include "dictionary.h"
#include "board.h"
#include "graphics.h"
#include "utils.h"


int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    srand(time(NULL));
    
    DictionaryEntry *dictionaryHash = loadDictionaryHash("mots_filtres.txt");
    if (!dictionaryHash) {
        fprintf(stderr, "Erreur lors du chargement du dictionnaire.\n");
        return EXIT_FAILURE;
    }
    
    int boardSize = 15;
    char **board = initBoard(boardSize);
    if (!board)
        return EXIT_FAILURE;
    
    // Plateau bonus (15x15) pour les multiplicateurs de points
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
    
    int totalPoints = 0;
    
    Resources res;
    if (initResources(&res) != 0) {
        freeBoard(board, boardSize);
        return EXIT_FAILURE;
    }
    
    // Initialisation du rack (chevalet) avec 7 lettres aléatoires
    char rack[8];
    for (int i = 0; i < 7; i++)
        rack[i] = drawRandomLetter();
    rack[7] = '\0';
    
    // Variables de gestion de la saisie utilisateur
    InputState currentState = STATE_IDLE;
    char inputBuffer[50] = "";
    int inputLength = 0;
    int selectedCellX = -1, selectedCellY = -1;
    int lastWordScore = 0;
    
    int boardDrawWidth  = WINDOW_WIDTH - 2 * BOARD_MARGIN;
    int boardDrawHeight = BOARD_HEIGHT - 2 * BOARD_MARGIN;
    float cellWidth  = (float)boardDrawWidth / boardSize;
    float cellHeight = (float)boardDrawHeight / boardSize;
    int gridThickness = 2;
    
    int rackAreaWidth = 300;
    SDL_Surface *btnSurface = TTF_RenderUTF8_Blended(res.inputFont, "Echanger", TEXT_COLOR);
    int btnW, btnH;
    SDL_Texture *tempTex = SDL_CreateTextureFromSurface(res.renderer, btnSurface);
    SDL_QueryTexture(tempTex, NULL, NULL, &btnW, &btnH);
    SDL_DestroyTexture(tempTex);
    SDL_FreeSurface(btnSurface);
    int buttonWidth = btnW + 10;
    int buttonHeight = btnH + 4;
    int buttonMargin = 10;
    int totalRackWidth = rackAreaWidth + buttonMargin + buttonWidth;
    int startXRack = (WINDOW_WIDTH - totalRackWidth) / 2;
    
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = true;
            
            if (currentState == STATE_IDLE) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int mouseX = e.button.x;
                    int mouseY = e.button.y;
                    if (mouseX >= BOARD_MARGIN && mouseX < (BOARD_MARGIN + boardDrawWidth) &&
                        mouseY >= BOARD_MARGIN && mouseY < (BOARD_MARGIN + boardDrawHeight)) {
                        selectedCellX = (mouseX - BOARD_MARGIN) / cellWidth;
                        selectedCellY = (mouseY - BOARD_MARGIN) / cellHeight;
                        currentState = STATE_INPUT_TEXT;
                        inputBuffer[0] = '\0';
                        inputLength = 0;
                        SDL_StartTextInput();
                    } else if (mouseY >= BOARD_HEIGHT && mouseY < (BOARD_HEIGHT + SCRABBLE_RACK_HEIGHT)) {
                        int buttonX = startXRack + rackAreaWidth + buttonMargin;
                        int buttonY = BOARD_HEIGHT + (SCRABBLE_RACK_HEIGHT - buttonHeight) / 2;
                        if (mouseX >= buttonX && mouseX < buttonX + buttonWidth &&
                            mouseY >= buttonY && mouseY < buttonY + buttonHeight) {
                            for (int i = 0; i < 7; i++)
                                rack[i] = drawRandomLetter();
                            rack[7] = '\0';
                        }
                        int bestMoveButtonX = buttonX + buttonWidth + 10;
                        int bestMoveButtonY = buttonY;
                        int bestMoveButtonWidth = 120;
                        int bestMoveButtonHeight = buttonHeight;
                        if (mouseX >= bestMoveButtonX && mouseX < bestMoveButtonX + bestMoveButtonWidth &&
                            mouseY >= bestMoveButtonY && mouseY < bestMoveButtonY + bestMoveButtonHeight) {
                            findBestMove(board, boardSize, dictionaryHash, rack, &totalPoints, bonusBoard);
                        }
                    }
                }
            }
            else if (currentState == STATE_INPUT_TEXT) {
                if (e.type == SDL_TEXTINPUT) {
                    if (inputLength + strlen(e.text.text) < sizeof(inputBuffer) - 1) {
                        strcat(inputBuffer, e.text.text);
                        inputLength = strlen(inputBuffer);
                        for (size_t i = 0; i < strlen(inputBuffer); i++)
                            inputBuffer[i] = toupper(inputBuffer[i]);
                    }
                } else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_BACKSPACE && inputLength > 0) {
                        inputBuffer[inputLength - 1] = '\0';
                        inputLength--;
                    } else if (e.key.keysym.sym == SDLK_RETURN) {
                        SDL_StopTextInput();
                        if (inputLength == 0) {
                            currentState = STATE_IDLE;
                        } else if (!isValidWordHash(inputBuffer, dictionaryHash)) {
                            fprintf(stderr, "Mot invalide: %s\n", inputBuffer);
                            inputBuffer[0] = '\0';
                            inputLength = 0;
                            currentState = STATE_IDLE;
                        } else if (inputLength == 1) {
                            if (canPlaceWord(inputBuffer, selectedCellX, selectedCellY, 'h', board, boardSize, rack, totalPoints)) {
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
                        } else {
                            currentState = STATE_INPUT_DIRECTION;
                        }
                    } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                        SDL_StopTextInput();
                        currentState = STATE_IDLE;
                    }
                }
            }
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
                                int tilesUsed = 0;
                                for (int i = 0; i < len; i++) {
                                    int x = selectedCellX, y = selectedCellY;
                                    if (dir == 'h')
                                        x += i;
                                    else
                                        y += i;
                                    int letterMultiplier = 1;
                                    if (board[y][x] == ' ') {
                                        tilesUsed++;
                                        int bonus = bonusBoard[y][x];
                                        switch(bonus) {
                                            case 1: wordMultiplier *= 3; break;
                                            case 2: wordMultiplier *= 2; break;
                                            case 3: letterMultiplier = 3; break;
                                            case 4: letterMultiplier = 2; break;
                                            default: break;
                                        }
                                    }
                                    score += getLetterScore(toupper(inputBuffer[i])) * letterMultiplier;
                                }
                                score *= wordMultiplier;
                                if (tilesUsed == 7)
                                    score += 50;
                                int perpendicularScore = 0;
                                for (int i = 0; i < len; i++) {
                                    int x = selectedCellX, y = selectedCellY;
                                    if (dir == 'h')
                                        x += i;
                                    else
                                        y += i;
                                    if (board[y][x] == ' ') {
                                        int perpWordMultiplier = 1;
                                        int perpScore = 0;
                                        if (dir == 'h') {
                                            int startRow = y;
                                            while (startRow > 0 && board[startRow - 1][x] != ' ')
                                                startRow--;
                                            int endRow = y;
                                            while (endRow < boardSize - 1 && board[endRow + 1][x] != ' ')
                                                endRow++;
                                            if (endRow - startRow + 1 > 1) {
                                                for (int r = startRow; r <= endRow; r++) {
                                                    if (r == y) {
                                                        int letterMult = 1;
                                                        int bonus = bonusBoard[y][x];
                                                        if (bonus == 3)
                                                            letterMult = 3;
                                                        else if (bonus == 4)
                                                            letterMult = 2;
                                                        if (bonus == 1)
                                                            perpWordMultiplier *= 3;
                                                        else if (bonus == 2)
                                                            perpWordMultiplier *= 2;
                                                        perpScore += getLetterScore(toupper(inputBuffer[i])) * letterMult;
                                                    } else {
                                                        perpScore += getLetterScore(toupper(board[r][x]));
                                                    }
                                                }
                                                perpendicularScore += perpScore * perpWordMultiplier;
                                            }
                                        } else {
                                            int startCol = x;
                                            while (startCol > 0 && board[y][startCol - 1] != ' ')
                                                startCol--;
                                            int endCol = x;
                                            while (endCol < boardSize - 1 && board[y][endCol + 1] != ' ')
                                                endCol++;
                                            if (endCol - startCol + 1 > 1) {
                                                int perpWordMultiplier = 1;
                                                int perpScore = 0;
                                                for (int c = startCol; c <= endCol; c++) {
                                                    if (c == x) {
                                                        int letterMult = 1;
                                                        int bonus = bonusBoard[y][x];
                                                        if (bonus == 3)
                                                            letterMult = 3;
                                                        else if (bonus == 4)
                                                            letterMult = 2;
                                                        if (bonus == 1)
                                                            perpWordMultiplier *= 3;
                                                        else if (bonus == 2)
                                                            perpWordMultiplier *= 2;
                                                        perpScore += getLetterScore(toupper(inputBuffer[i])) * letterMult;
                                                    } else {
                                                        perpScore += getLetterScore(toupper(board[y][c]));
                                                    }
                                                }
                                                perpendicularScore += perpScore * perpWordMultiplier;
                                            }
                                        }
                                    }
                                }
                                score += perpendicularScore;
                                lastWordScore = score;
                                totalPoints += score;
                                placeWord(inputBuffer, selectedCellX, selectedCellY, dir, board, rack);
                                for (int i = 0; i < len; i++) {
                                    int x = selectedCellX, y = selectedCellY;
                                    if (dir == 'h')
                                        x += i;
                                    else
                                        y += i;
                                    bonusBoard[y][x] = 0;
                                }
                            }
                        }
                        currentState = STATE_IDLE;
                    } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                        currentState = STATE_IDLE;
                    }
                }
            }
        }
        
        drawGrid(res.renderer, boardSize, boardDrawWidth, boardDrawHeight);
        drawBoard(res.renderer, res.boardFont, res.valueFont, board, boardSize, boardDrawWidth, boardDrawHeight, gridThickness);
        drawRack(res.renderer, res.rackFont, res.valueFont, rack, rackAreaWidth, startXRack, buttonMargin, buttonWidth, buttonHeight, res.inputFont);
        drawInputArea(res.renderer, res.inputFont, currentState, inputBuffer, totalPoints);
        
        {
            char scoreText[50];
            snprintf(scoreText, sizeof(scoreText), "Points: %d", lastWordScore);
            SDL_Surface *scoreSurface = TTF_RenderUTF8_Blended(res.boardFont, scoreText, TEXT_COLOR);
            if (scoreSurface) {
                SDL_Texture *scoreTexture = SDL_CreateTextureFromSurface(res.renderer, scoreSurface);
                int textW, textH;
                SDL_QueryTexture(scoreTexture, NULL, NULL, &textW, &textH);
                int posX = WINDOW_WIDTH - textW - 10;
                int posY = 10;
                SDL_Rect scoreRect = { posX, posY, textW, textH };
                SDL_RenderCopy(res.renderer, scoreTexture, NULL, &scoreRect);
                SDL_DestroyTexture(scoreTexture);
                SDL_FreeSurface(scoreSurface);
            }
        }
        {
            char totalText[50];
            snprintf(totalText, sizeof(totalText), "Total: %d", totalPoints);
            SDL_Surface *totalSurface = TTF_RenderUTF8_Blended(res.boardFont, totalText, TEXT_COLOR);
            if (totalSurface) {
                SDL_Texture *totalTexture = SDL_CreateTextureFromSurface(res.renderer, totalSurface);
                int totalW, totalH;
                SDL_QueryTexture(totalTexture, NULL, NULL, &totalW, &totalH);
                SDL_Rect totalRect = { 10, 10, totalW, totalH };
                SDL_RenderCopy(res.renderer, totalTexture, NULL, &totalRect);
                SDL_DestroyTexture(totalTexture);
                SDL_FreeSurface(totalSurface);
            }
        }
        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                char letter = board[y][x];
                if (letter != ' ') {
                    char text[2] = { letter, '\0' };
                    SDL_Surface *textSurface = TTF_RenderUTF8_Blended(res.boardFont, text, TEXT_COLOR);
                    if (textSurface) {
                        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(res.renderer, textSurface);
                        int textW, textH;
                        SDL_QueryTexture(textTexture, NULL, NULL, &textW, &textH);
                        int posX = BOARD_MARGIN + (int)(x * cellWidth + (cellWidth - textW) / 2);
                        int posY = BOARD_MARGIN + (int)(y * cellHeight + (cellHeight - textH) / 2);
                        SDL_Rect dstRect = { posX, posY, textW, textH };
                        SDL_RenderCopy(res.renderer, textTexture, NULL, &dstRect);
                        SDL_DestroyTexture(textTexture);
                        SDL_FreeSurface(textSurface);
                    }
                    char valueText[4];
                    sprintf(valueText, "%d", getLetterScore(letter));
                    SDL_Surface *valueSurface = TTF_RenderUTF8_Blended(res.valueFont, valueText, TEXT_COLOR);
                    if (valueSurface) {
                        SDL_Texture *valueTexture = SDL_CreateTextureFromSurface(res.renderer, valueSurface);
                        int valueW, valueH;
                        SDL_QueryTexture(valueTexture, NULL, NULL, &valueW, &valueH);
                        int valuePosX = BOARD_MARGIN + (int)(x * cellWidth) + (int)cellWidth - valueW - 2;
                        int valuePosY = BOARD_MARGIN + (int)(y * cellHeight) + (int)cellHeight - valueH - 2;
                        SDL_Rect valueRect = { valuePosX, valuePosY, valueW, valueH };
                        SDL_RenderCopy(res.renderer, valueTexture, NULL, &valueRect);
                        SDL_DestroyTexture(valueTexture);
                        SDL_FreeSurface(valueSurface);
                    }
                }
            }
        }
        SDL_RenderPresent(res.renderer);
        SDL_Delay(64);
    }
    
    cleanup(&res, dictionaryHash, board, boardSize);
    return EXIT_SUCCESS;
}

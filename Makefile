# Nom de l'exécutable
TARGET = scrabble

# Fichier source
SRC = scrabble.c

# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall -Wextra -std=c11 -I/usr/include/SDL2
LDFLAGS = -lSDL2 -lSDL2_ttf

# Règle par défaut : compilation du programme
all: $(TARGET)

# Compilation de l'exécutable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) -lm

# Nettoyage des fichiers objets et de l'exécutable
clean:
	rm -f $(TARGET)

# Nettoyage complet (y compris les fichiers temporaires de compilation)
distclean: clean
	rm -f *~ *.o


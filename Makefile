# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall -Wextra -std=c11 -I/usr/include/SDL2 -g

# Bibliothèques nécessaires
LIBS = -lSDL2 -lSDL2_ttf -lm

# Liste des fichiers source
SRCS = main.c dictionary.c board.c graphics.c utils.c

# Liste des fichiers objets (transforme les fichiers .c en .o)
OBJS = $(SRCS:.c=.o)

# Nom de l'exécutable
TARGET = scrabble

# Règle par défaut : compiler l'exécutable
all: $(TARGET)

# Règle pour compiler l'exécutable à partir des fichiers objets
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Règle pour compiler chaque fichier .c en .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage des fichiers objets et de l'exécutable
clean:
	rm -f $(OBJS) $(TARGET)

# Nettoyage complet (y compris les fichiers de sauvegarde éventuels)
distclean: clean
	rm -f *~

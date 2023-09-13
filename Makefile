CC = clang
CFLAGS = -Wall -Wextra -std=c11

SRC_FILES = \
 	keywords.c \
 	main.c \
 	lexer.c \
	#parser.c
OBJ_FILES = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC_FILES))
EXECUTABLE = main.out
BUILD_DIR = build

.PHONY: all clean

all: $(BUILD_DIR) $(EXECUTABLE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(EXECUTABLE): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@

# Removes clutter
clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE)
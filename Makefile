CC = clang
CFLAGS = -g -fstack-protector -Wextra -std=c17

SRC_FILES = \
  keywords.c \
  main.c \
  lexer.c \
  parser.c \
  dbgtools.c \
  semanalysis.c \
  vartable.c \
  memtracker.c \
  errors.c \
  compiler.c \
  generics/hashset.c \
  generics/utilities.c 

BUILD_DIR = build
OBJ_FILES = $(addprefix $(BUILD_DIR)/, $(notdir $(SRC_FILES:.c=.o)))

EXECUTABLE = main.out

.PHONY: all clean

all: $(BUILD_DIR) $(EXECUTABLE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Generic rule for compiling sources files in root 
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Generic rule for compiling source files in the generics directory
$(BUILD_DIR)/%.o: generics/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(EXECUTABLE): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ 

# Removes clutter
clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE)

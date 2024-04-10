CC = clang
# CFLAGS = -fsanitize=address -fno-omit-frame-pointer -fstack-protector -g  -Wextra -std=c17
CFLAGS = -g -Wextra -std=c17
# CFLAGS = -O3 -Wextra -std=c17

SRC_FILES = \
  main.c \
  parser/keywords.c \
  parser/lexer.c \
  parser/parser.c \
  parser/semanalysis.c \
  parser/vartable.c \
  parser/errors.c \
  misc/dbgtools.c \
  misc/memtracker.c \
  compiler/compiler.c \
  compiler/exprsimplifier.c \
  runtime/rtobjects.c \
  runtime/runtime.c \
  runtime/rtexchandler.c \
  runtime/identtable.c \
  runtime/stkmachine.c \
  runtime/gc.c \
  runtime/rtfunc.c \
  runtime/rtlists.c \
  runtime/rtmap.c \
  runtime/rtset.c \
  runtime/rtclass.c \
  runtime/rtstring.c \
  runtime/rttype.c \
  runtime/rtnumber.c \
  runtime/rtexception.c \
  runtime/filetable.c \
  rtlib/builtinfuncs.c \
  rtlib/builtinexception.c \
  rtlib/rtattrs.c \
  rtlib/rtattrslist.c \
  rtlib/rtattrsmap.c \
  rtlib/rtattrsset.c \
  rtlib/rtattrsstr.c \
  generics/hashset.c \
  generics/hashmap.c \
  generics/linkedlist.c \
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

# Generic rule for compiling source files in the /generics directory
$(BUILD_DIR)/%.o: generics/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Generic rule for compiling source files in the /misc directory
$(BUILD_DIR)/%.o: misc/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Generic rule for compiling source files in the /parser directory
$(BUILD_DIR)/%.o: parser/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Generic rule for compiling source files in the /compiler directory
$(BUILD_DIR)/%.o: compiler/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Generic rule for compiling source files in the /runtime directory
$(BUILD_DIR)/%.o: runtime/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Generic rule for compiling source files in the /rtlib directory
$(BUILD_DIR)/%.o: rtlib/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(EXECUTABLE): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ 

# Removes clutter
clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE)

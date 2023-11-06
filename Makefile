CC = gcc
CFLAGS = -g -fsanitize=address
LIBS = -lSDL -lSDLmain -lm -lz
INCLUDES = -I include/
SRC_DIR = src
SOURCES = $(wildcard $(SRC_DIR)/*/*/*.c $(SRC_DIR)/*/*.c $(SRC_DIR)/*.c)
OBJ_DIR = obj
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
EXECUTABLE = a.out

$(EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(INCLUDES) $(LIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(EXECUTABLE)

.PHONY: clean
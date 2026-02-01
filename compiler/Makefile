# Compiler and Flags
CC = gcc
# -I. tells the compiler to look in the current directory for headers
CFLAGS = -g -O0 -Wall -Wextra -I.

# List all your source files (.c)
# We include the paths relative to where this Makefile sits
SRCS = main.c \
utilities/utils.c \
symbol_table/symbol_table.c \
frontend/parsing/parsing.c \
frontend/tokenization/tokenize.c \
frontend/expression_creation/expressions.c \
backend/assembly_generator/x86_64/x86_64.c \
backend/assembly_generator/x86_64/evaluate_expr.c \
arena/arena.c \
error_handler/error_handler.c

# Automatically generate a list of object files (.o) from the source files
OBJS = $(SRCS:.c=.o)

# The name of your final executable
TARGET = my_compiler

# Default rule: build the compiler
all: $(TARGET)

# Link the object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Rule to compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)




# # Compiler and Flags
# CC = gcc
# # -I. tells the compiler to look in the current directory for headers
# # -g adds debug symbols for GDB
# CFLAGS = -Wall -Wextra -I. -g

# # List all your source files (.c)
# # We include the paths relative to where this Makefile sits
# SRCS = main.c \
# utilities/utils.c \
# symbol_table/symbol_table.c \
# frontend/parsing/parsing.c \
# frontend/tokenization/tokenize.c \
# frontend/expression_creation/expressions.c \
# backend/assembly_generator/x86_64/x86_64.c \
# backend/assembly_generator/x86_64/evaluate_expr.c \
# arena/arena.c \
# error_handler/error_handler.c

# # Automatically generate a list of object files (.o) from the source files
# OBJS = $(SRCS:.c=.o)

# # The name of your final executable
# TARGET = my_compiler

# # Default rule: build the compiler
# all: $(TARGET)

# # Link the object files into the final executable
# $(TARGET): $(OBJS)
# 	$(CC) -g $(OBJS) -o $(TARGET)

# # Rule to compile .c files into .o files
# %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# # Clean up build files
# clean:
# 	rm -f $(OBJS) $(TARGET)

# # Optional: build with optimization disabled for easier debugging
# debug: CFLAGS += -O0
# debug: clean all
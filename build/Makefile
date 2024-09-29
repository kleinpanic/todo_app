CC = gcc
CFLAGS = -Wall -g
OBJDIR = ../obj
SRCDIR = ../src
INCDIR = ../include
BUILDDIR = .
BINDIR = .

OBJS = $(OBJDIR)/main.o $(OBJDIR)/task.o
EXEC = $(BINDIR)/todo

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lncurses

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I $(INCDIR) -c $< -o $@

install: $(EXEC)
	@echo "Installing $(EXEC) to /usr/local/bin/todo..."
	@cp $(EXEC) /usr/local/bin/todo
	@echo "Installation complete."

clean:
	@echo "Cleaning up build artifacts..."
	rm -f $(OBJDIR)/*.o $(EXEC)
	@echo "Clean complete."

.PHONY: all clean install

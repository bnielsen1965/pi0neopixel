SRCDIR = src
OBJDIR = obj
BINDIR = bin

TARGET = pi0neopixel
TARGETLIBS =

IFLAGS =
LFLAGS = -lm

CC = gcc
CFLAGS =

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst %.c,%.o,$(notdir $(SOURCES)))

# build all
all: $(TARGET)

# build target
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(BINDIR)/$@ $(addprefix $(OBJDIR)/,$(OBJECTS)) $(TARGETLIBS) $(LFLAGS)

# build object files
$(OBJECTS): %.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $(OBJDIR)/$@

# clean up
.PHONY: clean
clean:
	rm -f $(BINDIR)/$(TARGET) $(addprefix $(OBJDIR)/,$(OBJECTS))

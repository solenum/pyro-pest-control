CC     =emcc
CFLAGS =-Wall -Wno-unused -O3 -r -std=c99 -lSDL2 --closure 1 -flto --llvm-lto 1 -s INLINING_LIMIT=1
ODIR   =obj
BDIR   =build
BIN    =rl.bc

DEPS   =$(wildcard *.h)
OBJ    =$(patsubst %.c,$(ODIR)/%.o,$(wildcard *.c))

all: files rl

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rl: $(OBJ)
	$(CC) -o $(BDIR)/$(BIN) $^ $(CFLAGS)

files:
	mkdir -p $(ODIR)
	mkdir -p $(BDIR)/data
	cp -r data $(BDIR)/

.PHONY: clean

clean:
	rm -rf $(ODIR)
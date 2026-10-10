# ---- toolchain ----
CC      := gcc
STD     := -std=c99
WARN    := -Wall -Wextra
OPT     := -O2
CFLAGS  := $(STD) $(WARN) $(OPT) $(shell pkg-config --cflags sdl2)
# NOTE: pkg-config emits -mwindows (hides console). For dev builds we want
# console output, so filter it out and force -mconsole.
LIBS    := $(filter-out -mwindows,$(shell pkg-config --libs sdl2)) -mconsole

# ---- local temp redirect (bug workaround, see PLAN-SDL.md §6) ----
TMPDIR_LOCAL := .tmp
$(shell mkdir -p $(TMPDIR_LOCAL) build dist)
export TMPDIR    := $(abspath $(TMPDIR_LOCAL))
export TEMP      := $(abspath $(TMPDIR_LOCAL))
export TMP       := $(abspath $(TMPDIR_LOCAL))
export GCC_TMPDIR := $(abspath $(TMPDIR_LOCAL))
export TMPDIR_LOCAL

# ---- layout: src -> build/*.o -> dist/lala.exe ----
SRC     := $(wildcard src/*.c)
OBJ     := $(patsubst src/%.c,build/%.o,$(SRC))
TARGET  := dist/lala.exe
ASSET_SRC := ../lala-js/public
ASSET_DST := dist/assets

all: $(TARGET)

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	mkdir -p dist
	$(CC) $(OBJ) -o $@ $(LIBS)

# Copy runtime assets (idempotent). Phase 0 needs GFX/TITLE.PCX at least.
assets:
	mkdir -p $(ASSET_DST)
	cp -r $(ASSET_SRC)/GFX $(ASSET_DST)/
	cp -r $(ASSET_SRC)/MAP $(ASSET_DST)/
	cp -r $(ASSET_SRC)/MUSIC $(ASSET_DST)/
	cp -r $(ASSET_SRC)/SFX $(ASSET_DST)/

run: $(TARGET) assets
	./$(TARGET)

clean:
	rm -f build/*.o $(TARGET)

distclean: clean
	rm -rf $(TMPDIR_LOCAL) build

.PHONY: all assets run clean distclean

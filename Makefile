# ---- toolchain ----
CC      := gcc
STD     := -std=c99
WARN    := -Wall -Wextra
OPT     := -O2
CFLAGS  := $(STD) $(WARN) $(OPT) $(shell pkg-config --cflags sdl2 SDL2_mixer)
# NOTE: pkg-config emits -mwindows (hides console). For dev builds we want
# console output, so filter it out and force -mconsole.
LIBS    := $(filter-out -mwindows,$(shell pkg-config --libs sdl2 SDL2_mixer)) -mconsole

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

# ---- phase 1 parity test (pure C, no SDL) ----
TEST_SRC := tests/test_assets.c
TEST_BIN := build/test_assets.exe
TEST_LIB_SRC := src/assets.c src/txt.c src/fnt.c src/wav.c src/map_load.c src/pcx.c

# ---- phase 2 tests (screen/font/render/keyboard, needs SDL) ----
TEST2_SRC := tests/test_phase2.c
TEST2_BIN := build/test_phase2.exe
TEST2_LIB_SRC := src/screen.c src/font.c src/render.c src/keyboard.c src/fnt.c src/pcx.c src/assets.c src/txt.c src/map_load.c src/wav.c

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

# ---- phase 3 tests (audio, needs SDL2_mixer) ----
TEST3_SRC := tests/test_audio.c
TEST3_BIN := build/test_audio.exe
TEST3_LIB_SRC := src/audio.c src/assets.c

test: assets $(TEST_BIN) $(TEST2_BIN) $(TEST3_BIN)
	./$(TEST_BIN)
	SDL_VIDEODRIVER=dummy ./$(TEST2_BIN)
	SDL_AUDIODRIVER=dummy ./$(TEST3_BIN)

$(TEST_BIN): $(TEST_SRC) $(TEST_LIB_SRC)
	mkdir -p build
	$(CC) $(STD) $(WARN) $(OPT) -Isrc $(TEST_SRC) $(TEST_LIB_SRC) -o $@

$(TEST2_BIN): $(TEST2_SRC) $(TEST2_LIB_SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc $(TEST2_SRC) $(TEST2_LIB_SRC) -o $@ $(LIBS)

$(TEST3_BIN): $(TEST3_SRC) $(TEST3_LIB_SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc $(TEST3_SRC) $(TEST3_LIB_SRC) -o $@ $(LIBS)

clean:
	rm -f build/*.o $(TARGET) $(TEST_BIN) $(TEST2_BIN) $(TEST3_BIN)

distclean: clean
	rm -rf $(TMPDIR_LOCAL) build

.PHONY: all assets run test clean distclean

# nTheme: needs the Ndless SDK's bin/ on PATH, then `make`.
EXE = nTheme
GCC = nspire-gcc
LD = nspire-ld
GENZEHN = genzehn

CFLAGS = -Wall -W -std=gnu99 -marm -Os -Isrc -Ibuild $(EXTRA_CFLAGS)
# Pull libsyscalls' allocator in before newlib's libc is scanned; recent Arm GNU
# toolchains otherwise link newlib's _malloc_r first and fail on the duplicate.
LDFLAGS = -Wl,-u,malloc -Wl,-u,abort -Wl,-u,_malloc_r
VERSION_HEADER = build/version.h
VERSION = v$(shell date +%Y-%m-%d)

SOURCES = $(wildcard src/*.c) src/third_party/stb_image.c
OBJECTS = $(patsubst src/%.c,build/%.o,$(SOURCES))

all: $(EXE).tns

build/third_party/stb_image.o: CFLAGS += -w

build/%.o: src/%.c $(VERSION_HEADER)
	@mkdir -p $(dir $@)
	$(GCC) $(CFLAGS) -c $< -o $@

# Regenerated every build, rewritten only when the date changes.
$(VERSION_HEADER): FORCE
	@mkdir -p build
	@line='#define NTHEME_VERSION "$(VERSION)"'; \
	if [ ! -f $@ ] || [ "$$(cat $@)" != "$$line" ]; then echo "$$line" > $@; fi

$(EXE).elf: $(OBJECTS)
	$(LD) $(LDFLAGS) $^ -o $@

$(EXE).tns: $(EXE).elf
	$(GENZEHN) --input $< --output $@.zehn --name "$(EXE)" --notice "nTheme $(VERSION)" --240x320-support true
	make-prg $@.zehn $@
	rm $@.zehn

clean:
	rm -rf build $(EXE).elf $(EXE).tns

FORCE:
.PHONY: all clean FORCE

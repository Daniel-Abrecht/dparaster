.SECONDARY:

HEADERS := $(shell find include -type f -name "*.h" -not -name ".*")
SOURCES := $(shell find src -iname "*.c")

SONAME = dparaster
MAJOR  = 0
MINOR  = 0
PATCH  = 0

prefix = /usr/local/

TYPE := release

ifdef debug
TYPE := debug
CFLAGS  += -O0 -g
LDFLAGS += -g
else
CFLAGS  += -O3
endif

ifdef asan
TYPE := $(TYPE)-asan
CFLAGS  += -fsanitize=address
LDFLAGS += -fsanitize=address
endif

ifndef dynamic
LDFLAGS_BIN += -static
endif

CFLAGS  += --std=c17
CFLAGS  += -Iinclude
CFLAGS  += -Wall -Wextra -pedantic -Werror
CFLAGS  += -fstack-protector-all
CFLAGS  += -Wno-missing-field-initializers

ifndef debug
CFLAGS  += -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections
endif

LDLIBS_BIN += -Llib/$(TYPE)/ -l$(SONAME)
LDLIBS += -lm

OBJECTS := $(patsubst %,build/$(TYPE)/o/%.o,$(SOURCES))

.PHONY: all clean get-bin get-lib install uninstall shell test

all: bin/$(TYPE)/rasterizer \
     lib/$(TYPE)/lib$(SONAME).a \
     lib/$(TYPE)/lib$(SONAME).so

get-bin:
	@echo bin/$(TYPE)/

get-lib:
	@echo lib/$(TYPE)/

bin/$(TYPE)/%: build/$(TYPE)/o/src/main/%.c.o lib/$(TYPE)/lib$(SONAME).so
	mkdir -p $(dir $@)
	$(CC) -o $@ $(LDFLAGS) $(LDFLAGS_BIN) $< $(LDLIBS_BIN) $(LDLIBS)

lib/$(TYPE)/lib$(SONAME).so: lib/$(TYPE)/lib$(SONAME).a
	ln -sf "lib$(SONAME).so" "$@.0"
	$(CC) -o $@ -Wl,--no-undefined -Wl,-soname,lib$(SONAME).so.$(MAJOR) --shared -fPIC $(LDFLAGS) -Wl,--whole-archive $^ -Wl,--no-whole-archive $(LDLIBS)

lib/$(TYPE)/lib$(SONAME).a: $(filter-out build/$(TYPE)/o/src/main/%,$(OBJECTS))
	mkdir -p $(dir $@)
	rm -f $@
	$(AR) q $@ $^

build/$(TYPE)/o/%.c.o: %.c makefile $(HEADERS)
	mkdir -p $(dir $@)
	$(CC) -fPIC -c -o $@ $(DFLAGS) $(CFLAGS) $<

clean:
	rm -rf build/$(TYPE)/ bin/$(TYPE)/ lib/$(TYPE)/

install:
	cp -r include/dparaster/./ "$(DESTDIR)$(prefix)/include/dparaster/"
	cp "lib/$(TYPE)/lib$(SONAME).so" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)"
	ln -sf "lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR).$(MINOR)"
	ln -sf "lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR)"
	ln -sf "lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so"

uninstall:
	rm -rf "$(DESTDIR)$(prefix)/include/dparaster/"
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)"
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR).$(MINOR)"
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR)"
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so"

shell:
	PROMPT_COMMAND='if [ -z "$$PS_SET" ]; then PS_SET=1; PS1="(dparaster) $$PS1"; fi' \
	LD_LIBRARY_PATH="$$PWD/lib/$(TYPE)/" \
	PATH="$$PWD/bin/$(TYPE)/:$$PATH" \
	  "$$SHELL"

speed ?= 50
demo:
	./script/bmpvid './bin/release/rasterizer -y $$(echo "$$(date +%s.%N) * $(speed)" | bc -l) -'

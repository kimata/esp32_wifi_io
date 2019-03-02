DIST_PATH  = ./dist/esp32-wifi-io
DIST_FILES = runtime.js main.js polyfills.js scripts.js styles.css

all: build $(addsuffix .gz,$(addprefix $(DIST_PATH)/,$(DIST_FILES)))
	@echo "*BEFORE"
	@du -shc $(addprefix $(DIST_PATH)/,$(DIST_FILES))
	@echo "*AFTER"
	@du -shc $(DIST_PATH)/*.gz

build:
	ng build --prod --base-href=/app/

%.gz : %
	gzip -c --best $< > $@

.SUFFIXES: .gz
.PHONY: all build



SOURCE_DIR  = src/
INSTALL_DIR ?= /usr/local/

BINARY  = messenger-gtk
SOURCES = messenger_gtk.c\
		  application.c\
		  event.c\
		  chat/messenger.c\
		  ui/chat_entry.c\
		  ui/message.c\
		  ui/messenger.c\
		  ui/new_platform.c
		  
HEADERS = 

LIBRARIES = gnunetutil gnunetchat
PACKAGES  = libhandy-1 gtk+-3.0 libnotify

GNU_CC ?= gcc
GNU_LD ?= gcc
GNU_RM ?= rm

CFLAGS  += -pedantic -Wall -Wextra -march=native -ggdb3
LDFLAGS += 

DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG -fwhole-program

SOURCE_FILES  = $(addprefix $(SOURCE_DIR), $(SOURCES))
OBJECT_FILES  = $(SOURCE_FILES:%.c=%.o)
HEADER_FILES  = $(addprefix $(SOURCE_DIR), $(HEADERS))
LIBRARY_FLAGS = $(addprefix -l, $(LIBRARIES))
PACKAGE_FLAGS = $(shell pkg-config --cflags --libs $(PACKAGES))

all: $(BINARY)

debug: CFLAGS += $(DEBUGFLAGS)
debug: $(BINARY)

release: CFLAGS += $(RELEASEFLAGS)
release: $(BINARY)

%.o: %.c
	$(GNU_CC) $(CFLAGS) -c $< -o $@ $(LIBRARY_FLAGS) $(PACKAGE_FLAGS)

$(BINARY): $(OBJECT_FILES)
	$(GNU_LD) $(LDFLAGS) $^ -o $@ $(LIBRARY_FLAGS) $(PACKAGE_FLAGS)

.PHONY: mobile

mobile: $(BINARY)
	./$(BINARY) --mobile

.PHONY: install

install:
	install -m 755 $(BINARY) $(addprefix $(INSTALL_DIR), bin/)

.PHONY: uninstall

uninstall:
	$(GNU_RM) -f $(addsuffix $(BINARY), $(addprefix $(INSTALL_DIR), bin/))

.PHONY: clean

clean:
	$(GNU_RM) -f $(BINARY)
	$(GNU_RM) -f $(OBJECT_FILES)

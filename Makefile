
SOURCE_DIR  = src/
INSTALL_DIR ?= /usr/local/

BINARY  = messenger-gtk
SOURCES = messenger_gtk.c\
		  application.c\
		  contact.c\
		  event.c\
		  chat/messenger.c\
		  ui/chat.c\
		  ui/chat_entry.c\
		  ui/contact_entry.c\
		  ui/contacts.c\
		  ui/invite_contact.c\
		  ui/message.c\
		  ui/messenger.c\
		  ui/new_contact.c\
		  ui/new_group.c\
		  ui/new_platform.c\
		  ui/new_profile.c\
		  ui/picker.c\
		  ui/profile_entry.c\
		  ui/send_file.c\
		  ui/settings.c
		  
HEADERS = 

LIBRARIES = gnunetchat
PACKAGES  = gnunetutil libhandy-1 gtk+-3.0 libnotify zbar libqrencode
INCLUDES  = submodules/gnome-characters/lib

GNU_CC ?= gcc
GNU_LD ?= gcc
GNU_RM ?= rm

CFLAGS  += -pedantic -Wall -Wextra -ggdb3
LDFLAGS += 

DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG

SOURCE_FILES  = $(addprefix $(SOURCE_DIR), $(SOURCES))
OBJECT_FILES  = $(SOURCE_FILES:%.c=%.o)
HEADER_FILES  = $(addprefix $(SOURCE_DIR), $(HEADERS))
LIBRARY_FLAGS = $(addprefix -l, $(LIBRARIES))
PACKAGE_FLAGS = $(shell pkg-config --cflags --libs $(PACKAGES))
INCLUDE_FLAGS = $(addprefix -I, $(INCLUDES))

all: $(BINARY)

debug: CFLAGS += $(DEBUGFLAGS)
debug: $(BINARY)

release: CFLAGS += $(RELEASEFLAGS)
release: $(BINARY)

%.o: %.c
	$(GNU_CC) $(CFLAGS) -c $< -o $@ $(LIBRARY_FLAGS) $(PACKAGE_FLAGS) $(INCLUDE_FLAGS)

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

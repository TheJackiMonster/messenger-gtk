
APP_ID = org.gnunet.Messenger

RESOURCES_DIR = resources/
SOURCE_DIR    = src/
INSTALL_DIR  ?= /usr/local/

BINARY  = messenger-gtk
SOURCES = messenger_gtk.c\
		  application.c\
		  contact.c\
		  event.c\
		  file.c\
		  resources.c\
		  chat/messenger.c\
		  ui/about.c\
		  ui/account_entry.c\
		  ui/accounts.c\
		  ui/chat_entry.c\
		  ui/chat.c\
		  ui/contact_entry.c\
		  ui/contact_info.c\
		  ui/contacts.c\
		  ui/delete_messages.c\
		  ui/file_load_entry.c\
		  ui/invite_contact.c\
		  ui/message.c\
		  ui/messenger.c\
		  ui/new_account.c\
		  ui/new_contact.c\
		  ui/new_group.c\
		  ui/new_lobby.c\
		  ui/new_platform.c\
		  ui/picker.c\
		  ui/send_file.c\
		  ui/settings.c

HEADERS = util.h

RESOURCES = css.gresource.xml\
			ui.gresource.xml

LIBRARIES = gnunetchat
PACKAGES  = gnunetutil\
			glib-2.0\
			gstreamer-1.0\
			gtk+-3.0\
			libhandy-1\
			libnotify\
			libqrencode\
			zbar
			
INCLUDES  = submodules/gnome-characters/lib

GNU_CC ?= gcc
GNU_LD ?= gcc
GNU_RM ?= rm

GLIB_COMPILE_RESOURCES ?= glib-compile-resources

CFLAGS  += -pedantic -Wall -Wextra -ggdb3 -Wno-overlength-strings
LDFLAGS += 

DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG

ICON_SIZES = 32\
			 64\
			 128\
			 256\
			 512

APPICON_DIR = $(addprefix $(RESOURCES_DIR), icon/)

RESOURCES_FILES = $(addprefix $(RESOURCES_DIR), $(RESOURCES))

SOURCE_FILES  = $(RESOURCES_FILES:%.gresource.xml=%.c)\
				$(addprefix $(SOURCE_DIR), $(SOURCES))

RESOURCES_HEADERS = $(RESOURCES_FILES:%.gresource.xml=%.h)

HEADER_FILES  = $(addprefix $(SOURCE_DIR), $(HEADERS))

OBJECT_FILES  = $(SOURCE_FILES:%.c=%.o)

LIBRARY_FLAGS = $(addprefix -l, $(LIBRARIES))
PACKAGE_FLAGS = $(shell pkg-config --cflags --libs $(PACKAGES))
INCLUDE_FLAGS = $(addprefix -I, $(INCLUDES))

all: $(BINARY)

%.c: %.gresource.xml
	$(GLIB_COMPILE_RESOURCES) --sourcedir=$(RESOURCES_DIR) $< --generate-source
	$(GLIB_COMPILE_RESOURCES) --sourcedir=$(RESOURCES_DIR) $< --generate-header

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

define install-icon
	install -Dm644 $(addprefix $(APPICON_DIR), full_color_$(1).png) $(addprefix $(INSTALL_DIR), share/icons/hicolor/$(1)x$(1)/apps/$(APP_ID).png)
endef

install:
	install -Dm755 $(BINARY) $(addprefix $(INSTALL_DIR), bin/)
	$(foreach SIZE,$(ICON_SIZES),$(call install-icon,$(SIZE));)
	install -Dm644 $(addprefix $(APPICON_DIR), full_color.svg) $(addprefix $(INSTALL_DIR), share/icons/hicolor/scalable/apps/$(APP_ID).svg)
	desktop-file-install --dir=$(addprefix $(INSTALL_DIR), share/applications/) $(addprefix $(RESOURCES_DIR), $(APP_ID).desktop)
	gtk-update-icon-cache -f -t $(addprefix $(INSTALL_DIR), share/icons/hicolor)

.PHONY: uninstall

define uninstall-icon
	$(GNU_RM) -f $(addprefix $(INSTALL_DIR), share/icons/hicolor/$(1)x$(1)/apps/$(APP_ID).png)
endef

uninstall:
	$(GNU_RM) -f $(addsuffix $(BINARY), $(addprefix $(INSTALL_DIR), bin/))
	$(foreach SIZE,$(ICON_SIZES),$(call uninstall-icon,$(SIZE));)
	$(GNU_RM) -f $(addprefix $(INSTALL_DIR), share/icons/hicolor/scalable/apps/$(APP_ID).svg)
	$(GNU_RM) -f $(addsuffix $(APP_ID).desktop, $(addprefix $(INSTALL_DIR), share/applications/))

.PHONY: clean

clean:
	$(GNU_RM) -f $(BINARY)
	$(GNU_RM) -f $(OBJECT_FILES)
	$(GNU_RM) -f $(RESOURCES_HEADERS)

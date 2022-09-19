
VERSION     = 0.7.0
APP_ID      = org.gnunet.Messenger
TARGET_NAME = messenger-gtk

RESOURCES_DIR = resources/
SOURCE_DIR    = src/
INSTALL_DIR  ?= /usr/local/

PACKAGE = $(TARGET_NAME)
BINARY  = $(TARGET_NAME)
SOURCES = messenger_gtk.c\
		  application.c\
		  contact.c\
		  event.c\
		  file.c\
		  resources.c\
		  ui.c\
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
			gstreamer-app-1.0\
			gtk+-3.0\
			libhandy-1\
			libnotify\
			libqrencode

INCLUDES  = submodules/gnome-characters/lib

DIST_FILES = submodules/gnome-characters/lib/emoji.h\
             Makefile\
             AUTHORS\
			 CHANGES.md\
             COPYING\
             README.md

GNU_CC  ?= gcc
GNU_LD  ?= gcc
GNU_RM  ?= rm
GNU_CP  ?= cp
GNU_TAR ?= tar

GLIB_COMPILE_RESOURCES ?= glib-compile-resources

CFLAGS  += -pedantic -Wall -Wextra -ggdb3 -Wno-overlength-strings
LDFLAGS += 

DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG

DIST_DIR = $(PACKAGE)-$(VERSION)/
DIST_TAR = $(PACKAGE)-$(VERSION).tar.gz

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

LIBRARY_FLAGS = $(addprefix -l, $(LIBRARIES)) $(shell pkg-config --libs $(PACKAGES))
PACKAGE_FLAGS = $(shell pkg-config --cflags $(PACKAGES))
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
	$(GNU_CC) $(CFLAGS) -c $< -o $@ $(PACKAGE_FLAGS) $(INCLUDE_FLAGS)

$(BINARY): $(OBJECT_FILES)
	$(GNU_LD) $(LDFLAGS) $^ -o $@ $(LIBRARY_FLAGS)

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
	mkdir -p $(addprefix $(INSTALL_DIR), share/appdata/)
	install -Dm644 $(addprefix $(RESOURCES_DIR), $(APP_ID).appdata.xml) $(addprefix $(INSTALL_DIR), share/appdata/)
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
	$(GNU_RM) -f $(addsuffix $(APP_ID).appdata.xml, $(addprefix $(INSTALL_DIR), share/appdata/))

.PHONY: dist

dist: clean
	mkdir $(DIST_DIR)
	$(foreach INCLUDE_DIR,$(INCLUDES),mkdir -p $(addprefix $(DIST_DIR), $(INCLUDE_DIR));)
	$(GNU_CP) -r $(SOURCE_DIR) $(DIST_DIR)
	$(GNU_CP) -r $(RESOURCES_DIR) $(DIST_DIR)
	$(GNU_RM) -f $(addprefix $(addprefix $(DIST_DIR), $(RESOURCES_DIR)), ui/*.ui~)
	$(foreach DIST_FILE,$(DIST_FILES),$(GNU_CP) $(DIST_FILE) $(addprefix $(DIST_DIR), $(DIST_FILE));)
	$(GNU_TAR) -czf $(DIST_TAR) $(DIST_DIR)
	$(GNU_RM) -r $(DIST_DIR)

.PHONY: clean

clean:
	$(GNU_RM) -f $(BINARY)
	$(GNU_RM) -f $(OBJECT_FILES)
	$(GNU_RM) -f $(RESOURCES_HEADERS)
	$(GNU_RM) -f $(DIST_TAR)

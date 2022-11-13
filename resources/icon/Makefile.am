# This Makefile.am is in the public domain

theme=hicolor
themedir=$(datadir)/icons/$(theme)
theme_iconid=org.gnunet.Messenger
theme_iconsizes = \
  512 \
  256 \
  128 \
  64 \
  32

install-data-local:
	@$(NORMAL_INSTALL)
	for iconsize in ${theme_iconsizes}; do \
	  install -Dm644 "full_color_$${iconsize}.png" \
	    "${themedir}/$${iconsize}x$${iconsize}/apps/${theme_iconid}.png"; \
	done
	install -Dm644 "full_color.svg" \
	  "${themedir}/scalable/apps/${theme_iconid}.svg"
	gtk-update-icon-cache -f -t ${themedir}

uninstall-hook:
	for iconsize in ${theme_iconsizes}; do \
	  rm -f "${themedir}/$${iconsize}x$${iconsize}/apps/${theme_iconid}.png"; \
	done
	rm -f "${themedir}/scalable/apps/${theme_iconid}.svg"

EXTRA_DIST = \
  full_color_512.png \
  full_color_256.png \
  full_color_128.png \
  full_color_64.png \
  full_color_32.png \
  full_color.svg

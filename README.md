# messenger-gtk

A GTK based GUI for the Messenger service of GNUnet.

## Build & Installation

The following dependencies are required and need to be installed to build the application:

 - [gnunet](https://git.gnunet.org/gnunet.git/): For using general GNUnet datatypes
 - [libgnunetchat](https://git.gnunet.org/libgnunetchat.git/): For chatting via GNUnet messenger
 - [gtk3](https://gitlab.gnome.org/GNOME/gtk): For the general UI design
 - [libhandy](https://gitlab.gnome.org/GNOME/libhandy): For responsive UI widgets
 - [libnotify](https://gitlab.gnome.org/GNOME/libnotify): For notifications
 - [qrencode](https://github.com/fukuchi/libqrencode): For generating QR codes to share credentials
 - [zbar](https://github.com/mchehab/zbar): For scanning QR codes via camera
 - [gstreamer](https://gitlab.freedesktop.org/gstreamer): For recording and playing voice messages

As additional step you will need to load all required git submodules via `git submodule init` and `git submodule update`. It is also possible to just add the `--recursive` flag while cloning the repository to do that automatically.

Here is the list of the used submodules:

 - [gnome-characters](https://gitlab.gnome.org/GNOME/gnome-characters): For the emoji picker

Then you can simply use the provided Makefile as follows:

 - `make` to just compile everything with default parameters
 - `make clean` to cleanup build files in case you want to recompile
 - `make debug` to compile everything with debug parameters
 - `make release` to compile everything with build optimizations enabled
 - `make install` to install the compiled files (you might need sudo permissions to install)

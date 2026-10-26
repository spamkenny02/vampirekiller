# Installer SGDK, puis: make GDK=/chemin/vers/sgdk
ifndef GDK
$(error Definir GDK vers une installation SGDK, ex. make GDK=/opt/sgdk)
endif
# Compile project C sources with an explicit language version on every GCC host.
EXTRA_FLAGS += -std=c99
include $(GDK)/makefile.gen

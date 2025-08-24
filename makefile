# ----------------------------
# Program Options
# ----------------------------

NAME = SMS
ICON = icon.png
DESCRIPTION = "Sega Master System Emulator for TI-84 Plus CE"
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# Source files
SRC = src/main.c src/core/sms_single.c

# ----------------------------

include $(shell cedev-config --makefile)

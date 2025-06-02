# ----------------------------
# Makefile Options
# ----------------------------

NAME = PONG
ICON = icon.png
DESCRIPTION = "Pong84 v1.0.0 by Warren James"
COMPRESSED = YES
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)

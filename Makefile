#
# (c) 2016 42Bastian Schick
#

CFLAGS=-Wall -Os

# prevent false warning
CFLAGS += -Wno-maybe-uninitialized

dump2ascii: dump2ascii.c
	$(CC) $(CFLAGS) $< -o $@

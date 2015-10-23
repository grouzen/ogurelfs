
target = ogurelfs
srcs = src/ogurel.c src/fuseops.c
objs = src/ogurel.o src/fuseops.o
headers = src/fuseops.h src/ogurel.h

LDFLAGS += `pkg-config fuse --libs` `./python_ldflags.py`
CFLAGS += -std=gnu99 -Wall `pkg-config fuse --cflags` `./python_cflags.py`

.PHONY: all clean

all: $(objs)
	gcc -o $(target) $(objs) $(headers) $(LDFLAGS) $(CFLAGS)

cppdump: $(srcs)
	gcc -dM -E $(srcs) $(LDFLAGS) $(CFLAGS)

$(objs): $(headers)

clean:
	-rm -fv $(objs) $(target)


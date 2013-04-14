
target = ogurelfs
objs = ogurel.o
headers =

LDFLAGS += `pkg-config fuse --libs` `./python_ldflags.py`
CFLAGS += -std=c99 -Wall -g3 `pkg-config fuse --cflags` `./python_cflags.py`

.PHONY: all clean

all: $(objs)
	gcc -o $(target) $(objs) $(LDFLAGS) $(CFLAGS)

$(objs): $(headers)

clean:
	-rm -fv $(objs) $(target)


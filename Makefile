
target = ogurelfs
objs = ogurel.o
headers =

LDFLAGS += `pkg-config fuse --libs`
CFLAGS += -Wall -g3 `pkg-config fuse --cflags`

.PHONY: all clean

all: $(objs)
	gcc -o $(target) $(objs) $(LDFLAGS) $(CFLAGS)

$(objs): $(headers)

clean:
	-rm -fv $(objs) $(target)


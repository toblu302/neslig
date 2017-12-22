src = $(wildcard src/*.c)
obj = $(src:.c=.o)

LDFLAGS = -lSDL2

NESlig: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) NESlig

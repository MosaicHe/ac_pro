OBJS := server

server:main.c event.c
	gcc -o $@ $^ -Wall -g -lpthread

clean:
	$(RM) $(OBJS) .*.sw? *.c.bak  *.h.bak


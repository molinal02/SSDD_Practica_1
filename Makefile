CC=gcc
CFLAGS=-g -Wall
OBJS_SERVER=servidor.o libimplserv.a

all:  $(OBJS_SERVER)
	$(CC) $(CFLAGS) -o servidor_pf $(OBJS_SERVER) -lpthread

libimplserv.a: impl_serv.o
	ar rc libimplserv.a impl_serv.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS_SERVER) impl_serv.o servidor_pf
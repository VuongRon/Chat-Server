PORT = 57614
CFLAGS = -DPORT=\$(PORT) -Wall -Werror -std=c99

friends_server: friends_server.o friends.o 
	gcc $(CFLAGS) -o friends_server friends_server.o friends.o

friends_server.o: friends_server.c friends.h
	gcc $(CFLAGS) -c friends_server.c

friends.o: friends.c friends.h
	gcc $(CFLAGS) -c friends.c

clean: 
	rm friends_server *.o

# Compileaza ambele coduri sursa
all: server subscriber

# Compileaza server.c
server: server.c
	gcc server.c -o server -lm

# Compileaza client.c
subscriber: client.c
	gcc client.c -o subscriber

clean:
	rm -f server subscriber

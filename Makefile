all:server

server: server.c
	${CC}  -pthread -o server server.c
server-debug: server.c
	${CC} -DDEBUG -pthread -o server server.c
clean:
	rm server
run:all
	./server 8080 ~/comp30023-2018-project-1

debug:server-debug
	./server 8080 ~/comp30023-2018-project-1

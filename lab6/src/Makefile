all: lab6-server lab6-client

lab6-server: lab6-server.c
	gcc -o lab6-server lab6-server.c -lpthread

lab6-client: lab6-client.c
	gcc -o lab6-client lab6-client.c -lpthread

clean:
	rm -f lab6-client lab6-server

install: all
	mkdir -p /opt/lab6
	cp lab6-server lab6-client /opt/lab6/
	ln -sf /opt/lab6/lab6-server /usr/bin/lab6-server
	ln -sf /opt/lab6/lab6-client /usr/bin/lab6-client

uninstall:
	rm -rf /opt/lab5
	rm -f /usr/bin/lab6-client /usr/bin/lab6-server

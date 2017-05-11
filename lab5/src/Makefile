lab5: lab5.c
	gcc lab5.c -o lab5

clean:
	rm -f lab5 lab5.o

install: lab5
	mkdir -p /opt/lab5
	cp lab5 /opt/lab5/lab5
	ln -sf /opt/lab5/lab5 /usr/bin/lab5

uninstall:
	rm -rf /opt/lab5
	rm -f /usr/bin/lab5

lab4: lab4.o pr_a.o pr_b.o lab4.h
	gcc lab4.o pr_a.o pr_b.o -o lab4 -lm

pr_a.o: pr_a.c
	gcc -c pr_a.c

pr_b.o: pr_b.c
	gcc -c pr_b.c

lab4.o: lab4.c lab4.h
	gcc -c lab4.c

clean:
	rm -f lab4 lab4.o pr_a.o pr_b.o

install: lab4
	cp lab4 bin/lab4

uninstall: 
	rm -f bin/lab4

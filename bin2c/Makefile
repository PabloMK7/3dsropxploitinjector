all: bin2c.exe

bin2c.exe: main.c
	gcc -o main.o -c main.c
	gcc -o bin2c.exe main.o

clean:
	@rm -f main.o bin2c.exe
	@echo "all cleaned up !"

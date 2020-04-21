gcc -D_GNU_SOURCE -o lzss.o -c lzss.c
gcc -o main.o -c main.c
gcc -o compress.exe lzss.o main.o
pause
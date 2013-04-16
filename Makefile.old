all: capturer_mmap capturer_read 
#iewer

capturer_mmap: capturer_mmap.c
	gcc -O2  -o capturer_mmap capturer_mmap.c

capturer_read: capturer_read.c
	gcc -O2  -o capturer_read capturer_read.c
	
viewer: viewer.c
	gcc -O2 -static -fPIC -L/usr/lib/x86_64-linux-gnu -L/lib/x86_64-linux-gnu -lX11 -lXext -o viewer viewer.c

clean:
	rm -f capturer_mmap
	rm -f capturer_read
	rm -f viewer

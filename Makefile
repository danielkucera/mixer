all: mixer 

capturer_read: mixer.c
	gcc -O2  -o mixer mixer.c
	
clean:
	rm -f mixer

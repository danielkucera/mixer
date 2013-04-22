all: mixer 

mixer: mixer.c
	gcc -O2  -o mixer mixer.c -lpthread -lrt  
	
clean:
	rm -f mixer

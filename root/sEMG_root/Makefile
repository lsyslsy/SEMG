#sEMG app Makefile
#By HY
#CC = gcc
CC=arm-linux-gcc
OBJS = main.o socket.o collect.o time.o led.o
LIBS = -lpthread -lm
app:$(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)
.c.o:
	$(CC) -c $<
main.o:main.h socket.h collect.h time.h led.h
socket.o:main.h socket.h led.h
collect.o:main.h collect.h led.h
time.o:main.h time.h led.h
led.o:led.h
.PHONY:clean
clean:
	-rm app $(OBJS)

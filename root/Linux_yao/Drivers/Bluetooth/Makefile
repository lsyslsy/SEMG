
TARGETS := discover-client l2cap-client l2cap-server register-service rfcomm-client rfcomm-server semg-server semg-client simplescan
#OBJS := l2cap.o
LIBS := -lbluetooth
CFLAGS := -Wall

all: $(TARGETS)
#bluetooth: $(OBJS)
discover-client: discover-client.c
	$(CC) $(CFLAGS)  $^ $(LIBS)  -o $@
l2cap-client: l2cap-client.c
	$(CC) $(CFLAGS)  $^ $(LIBS)  -o $@
l2cap-server: l2cap-server.c
	$(CC) $(CFLAGS)  $^ $(LIBS)  -o $@
register-service: register-service.c	
	$(CC) $(CFLAGS)  $^ $(LIBS)  -o $@
rfcomm-client: rfcomm-client.c
	$(CC) $(CFLAGS)  $^ $(LIBS)  -o $@
rfcomm-server: rfcomm-server.c
	$(CC) $(CFLAGS)  $^ $(LIBS)  -o $@
semg-server: semg-server.c semg.h
	$(CC) $(CFLAGS)  $^ $(LIBS)  -o $@
semg-client: semg-client.c semg.h
	$(CC) $(CFLAGS)  $^ $(LIBS)  -o $@
simplescan: simplescan.c
	$(CC) $(CFLAGS)  $^ $(LIBS)  -o $@

install:
	cp $(TARGETS) -t /home/yao/temp

clean:
	rm $(TARGETS)

.PHONY: clean all

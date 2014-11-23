
TARGETS := discover-client l2cap-client l2cap-server register-service rfcomm-client rfcomm-server semg-server semg-client simplescan
#OBJS := l2cap.o
LIBS := -lbluetooth -lrt
CFLAGS := -Wall
LD_PATH:=-L/usr/local/bluez/lib 
IN_PATH:=-I/usr/local/bluez/include

all: $(TARGETS)
#bluetooth: $(OBJS)
discover-client: discover-client.c
	$(CC) $(CFLAGS)  $^ $(LIBS) $(LD_PATH) $(IN_PATH) -o $@
l2cap-client: l2cap-client.c
	$(CC) $(CFLAGS)  $^ $(LIBS) $(LD_PATH) $(IN_PATH) -o $@
l2cap-server: l2cap-server.c
	$(CC) $(CFLAGS)  $^ $(LIBS) $(LD_PATH) $(IN_PATH) -o $@
register-service: register-service.c	
	$(CC) $(CFLAGS)  $^ $(LIBS) $(LD_PATH) $(IN_PATH) -o $@
rfcomm-client: rfcomm-client.c
	$(CC) $(CFLAGS)  $^ $(LIBS) $(LD_PATH) $(IN_PATH) -o $@
rfcomm-server: rfcomm-server.c
	$(CC) $(CFLAGS)  $^ $(LIBS) $(LD_PATH) $(IN_PATH) -o $@
semg-server: semg-server.c semg.h
	$(CC) $(CFLAGS)  $^ $(LIBS) $(LD_PATH) $(IN_PATH) -o $@
semg-client: semg-client.c semg.h
	$(CC) $(CFLAGS)  $^ $(LIBS) $(LD_PATH) $(IN_PATH) -o $@
simplescan: simplescan.c
	$(CC) $(CFLAGS)  $^ $(LIBS) $(LD_PATH) $(IN_PATH) -o $@

install:
	cp $(TARGETS) -t /forlinx/share

clean:
	rm $(TARGETS)

.PHONY: clean all

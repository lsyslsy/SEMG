
CFLAGS = -Wall  -g
TARGET = sEMG_DLL sEMG_test
OBJS = Baseline_corrector.o Butterworth.o RLS.o sEMG_DLL.o socket.o utilities.o
SOURCES = Baseline_corrector.cpp Butterworth.cpp RLS.cpp sEMG_DLL.cpp socket.cpp utilities.cpp
# INCPATH =
# LIBPATH =
# LIBS =


all: $(TARGET)

sEMG_DLL: $(OBJS)

test: $(OBJS)
	sEMG_test

clean:
	rm sEMG_DLL

.PHONY: clean all
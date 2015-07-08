
CFLAGS = -Wall  -g -pthread -std=c++11
TARGET = sEMG_DLL sEMG_test
OBJS = Baseline_corrector.o Butterworth.o RLS.o sEMG_DLL.o socket.o utilities.o
SOURCES = Baseline_corrector.cpp Butterworth.cpp RLS.cpp sEMG_DLL.cpp socket.cpp utilities.cpp
# INCPATH =
# LIBPATH =
# LIBS =


all: $(TARGET)

sEMG_DLL: $(OBJS)


sEMG_test: $(OBJS) sEMG_test.cpp
	g++ -o $@ $^

clean:
	rm sEMG_DLL

.PHONY: clean all
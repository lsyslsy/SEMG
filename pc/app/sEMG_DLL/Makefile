
CFLAGS = -Wall  -g -pthread -std=c++11
TARGET = sEMG_DLL sEMG_test
OBJS = Baseline_corrector.o FOURIER.o Butterworth.o RLS.o sEMG_DLL.o socket.o utilities.o
SOURCES = Baseline_corrector.cpp FOURIER.cpp Butterworth.cpp RLS.cpp sEMG_DLL.cpp socket.cpp utilities.cpp
# INCPATH =
# LIBPATH =
# LIBS =


# all: sEMG_test

# sEMG_DLL: $(OBJS)


sEMG_test: $(SOURCES) sEMG_test.cpp
	g++ $(CFLAGS) -o $@ $^

clean:
	rm sEMG_test

.PHONY: clean all
CXXFLAGS=-DUNIT_TEST -std=c++11

all: test run

test: test_main.o test_time.o test_can.o dtc_tests.o ../src/dtc.o OBDMessage_tests.o ../src/OBDMessage.o
	g++ -o $@ $^

run:
	./test

# Use the rerun command to continuously build and run the unit tests
watch:
	rerun -x -p '{*.cpp,*.h,Makefile}' -d '.,../src' make

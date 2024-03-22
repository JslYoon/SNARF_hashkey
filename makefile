all: main

main: example.cpp 
	g++ -std=c++17 -O3 -w -fpermissive -I /Users/lucas/C++_lib/1.79.0/include example.cpp -o example.out

clean:
	rm example.out
	rm workload_tests.out
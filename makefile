clean:
	rm -f code
make:
	g++ mips_sim.cpp -o code -std=c++14 -O2
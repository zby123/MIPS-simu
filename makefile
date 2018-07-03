clean:
	rm -f mips
make:
	g++ mips_sim.cpp -o mips -std=c++14 -O2
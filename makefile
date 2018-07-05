make:
	g++ mips_sim.cpp -o mips -std=c++11 -O2
clean:
	rm -f mips
make:
	g++ mips_sim.cpp -o mips -std=c++11 -g
clean:
	rm -f mips
#include <iostream>
#include <cstring>
#include <string>
#include "mips.hpp"

char st[100];

int main(int argc, char *argv[]){
	mips mip;
	mip.init(argv[1]);
	mip.run();
	return 0;
}
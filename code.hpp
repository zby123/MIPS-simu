#ifndef CODE_HPP
#define CODE_HPP

#include "tokenscanner.hpp"
#include <string>
#include <map>
#include <vector>
using namespace std;

map<string, int> op_typ;
map<string, int> reg_name;
map<string, int> dat_typ;

void codeinit(){
	dat_typ[".align"] = 1;
	dat_typ[".ascii"] = 2;
	dat_typ[".asciiz"] = 3;
	dat_typ[".byte"] = 4;
	dat_typ[".half"] = 5;
	dat_typ[".word"] = 6;
	dat_typ[".space"] = 7;
	for(int i = 0; i < 32; i++) {
		string tmp = '$' + to_string(i);
		reg_name[tmp] = i;
	}
	reg_name["$zero"] = 0;
	reg_name["$at"] = 1;
	reg_name["$v0"] = 2;
	reg_name["$v1"] = 3;
	for(int i = 0; i < 4; i++) {
		string tmp = "$a" + to_string(i);
		reg_name[tmp] = i + 4;
	}
	for(int i = 0; i < 8; i++) {
		string tmp = "$t" + to_string(i);
		reg_name[tmp] = i + 8;
	}
	for(int i = 0; i < 8; i++) {
		string tmp = "$s" + to_string(i);
		reg_name[tmp] = i + 16;
	}
	reg_name["$t8"] = 24; reg_name["$t9"] = 25;
	reg_name["$k0"] = 26; reg_name["$k1"] = 27;
	reg_name["$gp"] = 28; reg_name["$sp"] = 29;
	reg_name["$fp"] = 30; reg_name["$ra"] = 31;
	reg_name["$hi"] = 32; reg_name["$lo"] = 33; reg_name["$pc"] = 34;
	op_typ["add"] = 0; op_typ["addu"] = 1;
	op_typ["addiu"] = 2; op_typ["sub"] = 3;
	op_typ["subu"] = 4; op_typ["mul"] = 5;
	op_typ["mulu"] = 6; op_typ["div"] = 9;
	op_typ["divu"] = 10; op_typ["xor"] = 13;
	op_typ["xoru"] = 14; op_typ["neg"] = 15;
	op_typ["negu"] = 16; op_typ["rem"] = 17;
	op_typ["remu"] = 18; op_typ["li"] = 19;
	op_typ["seq"] = 20; op_typ["sge"] = 21;
	op_typ["sgt"] = 22; op_typ["sle"] = 23;
	op_typ["slt"] = 24; op_typ["sne"] = 25;
	op_typ["b"] = 26; op_typ["beq"] = 27;
	op_typ["bne"] = 28; op_typ["bge"] = 29;
	op_typ["ble"] = 30; op_typ["bgt"] = 31;
	op_typ["blt"] = 32; op_typ["beqz"] = 33;
	op_typ["bnez"] = 34; op_typ["bgez"] = 35;
	op_typ["bgtz"] = 36; op_typ["bltz"] = 37;
	op_typ["j"] = 38; op_typ["jr"] = 39;
	op_typ["jal"] = 40; op_typ["jalr"] = 41;
	op_typ["la"] = 42; op_typ["lb"] = 43;
	op_typ["lh"] = 44; op_typ["lw"] = 45;
	op_typ["sb"] = 46; op_typ["sh"] = 47;
	op_typ["sw"] = 48; op_typ["move"] = 49;
	op_typ[]
}

class code{
public:
	int opr_type;
	int arg[3], offset;
	string lable;

	code(){

	}

	code(const Tokenscanner &buf){
		string tmp;
		tmp = buf.dat[0];
		opr_type = op_typ[tmp];
	}
};

#endif
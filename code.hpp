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
	reg_name["$lo"] = 32; reg_name["$hi"] = 33; reg_name["$pc"] = 34;
	op_typ["add"] = 0; op_typ["addu"] = 1;
	op_typ["addiu"] = 2; op_typ["sub"] = 3;
	op_typ["subu"] = 4; op_typ["mul"] = 5;
	op_typ["mulu"] = 7; op_typ["div"] = 9;
	op_typ["divu"] = 11; op_typ["xor"] = 13;
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
	op_typ["bnez"] = 34; op_typ["blez"] = 35;
	op_typ["bgez"] = 36;
	op_typ["bgtz"] = 37; op_typ["bltz"] = 38;
	op_typ["j"] = 39; op_typ["jr"] = 40;
	op_typ["jal"] = 41; op_typ["jalr"] = 42;
	op_typ["la"] = 43; op_typ["lb"] = 44;
	op_typ["lh"] = 45; op_typ["lw"] = 46;
	op_typ["sb"] = 47; op_typ["sh"] = 48;
	op_typ["sw"] = 49; op_typ["move"] = 50;
	op_typ["mfhi"] = 51; op_typ["mflo"] = 52;
	op_typ["nop"] = 53; op_typ["syscall"] = 54;
}

class code{
public:
	int opr_type;
	int arg[3], offset;
	string label;

	code(){

	}

	code(const Tokenscanner &buf){
		string tmp;
		tmp = buf.dat[0];
		opr_type = op_typ[tmp];
		if (opr_type >= 5 && opr_type <= 11) {
			if (buf.dat.size() == 3) opr_type ++;
		}
		switch (opr_type) {
			case 0: case 1: case 3: case 4: case 5: case 7: case 9: case 11: case 13: case 14: case 17: case 18: case 20: case 21: case 22: case 23: case 24: case 25:
				arg[0] = reg_name[buf.dat[1]];
				arg[1] = reg_name[buf.dat[2]];
				if (buf.dat[3][0] == '$') {
					arg[2] = reg_name[buf.dat[3]];
					offset = 1;
				}
				else {
					arg[2] = stoi(buf.dat[3]);
					offset = 0;
				}
				break;
			case 2: 
				arg[0] = reg_name[buf.dat[1]];
				arg[1] = reg_name[buf.dat[2]];
				arg[2] = stoi(buf.dat[3]);
				offset = 0;
				break;
			case 6: case 8: case 10: case 12: case 15: case 16: case 19: case 50:
				arg[0] = reg_name[buf.dat[1]];
				if (buf.dat[2][0] == '$') {
					arg[1] = reg_name[buf.dat[2]];
					offset = 1;
				}
				else {
					arg[1] = stoi(buf.dat[2]);
					offset = 0;
				}
				break;
			case 26: case 39: case 41:
				label = buf.dat[1];
				break;
			case 27: case 28: case 29: case 30: case 31: case 32:
				arg[0] = reg_name[buf.dat[1]];
				if (buf.dat[2][0] == '$') {
					arg[1] = reg_name[buf.dat[2]];
					offset = 1;
				}
				else {
					arg[1] = stoi(buf.dat[2]);
					offset = 0;
				}
				label = buf.dat[3];
				break;
			case 33: case 34: case 35: case 36: case 37: case 38:
				arg[0] = reg_name[buf.dat[1]];
				label = buf.dat[2];
				break;
			case 40: case 42: case 51: case 52:
				arg[0] = reg_name[buf.dat[1]];
				break;
			case 43: case 44: case 45: case 46: case 47: case 48: case 49:
				arg[0] = reg_name[buf.dat[1]];
				if (buf.dat.size() == 4) {
					arg[1] = reg_name[buf.dat[3]];
					offset = stoi(buf.dat[2]);
				}
				else {
					if (buf.dat[2][0] == '$') {
						arg[1] = reg_name[buf.dat[2]];
					}
					else label = buf.dat[2];
				}
				break;
		}
	}
};

#endif
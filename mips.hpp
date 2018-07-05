#ifndef MIPS_HPP
#define MIPS_HPP

#include <iostream>
#include <cstdio>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include "tokenscanner.hpp"
#include "code.hpp"
#include "pipeline.hpp"
#include "cpu.hpp"
#define Src(i) (cur.offset?reg[cur.arg[i]]:cur.arg[i])
using namespace std;

class mips {

	map<string, int> data_label;
	map<string, int> text_label;
	vector<string> n_label;
	pipeline pip;
	CPU cpu;

public:

	void init(const char* filename) {
		ifstream inf(filename);
		string tmp, tmp2;
		char line[1001];
		tmp = "";
		codeinit();
		cpu.init();
		bool is_data = false;
		Tokenscanner buf;
		int i, j;
		while (!inf.eof()) {
			inf.getline(line, 1000);
			buf.process(line);
			if (buf.dat.size() == 0) continue;
			if (buf.dat[0] == ".data") is_data = true;
			else if (buf.dat[0] == ".text") is_data = false;
			else if (buf.dat[0][buf.dat[0].length() - 1] == ':') {
				tmp = buf.dat[0]; tmp.pop_back();
				n_label.push_back(tmp);
			}
			else if (is_data) {
				if (buf.dat[0][buf.dat[0].length() - 1] == ':') {
					tmp = buf.dat[0]; tmp.pop_back();
					n_label.push_back(tmp);
				}
				else {
					int typ = dat_typ[buf.dat[0]];
					int tt, tn, i, j;
					switch (typ) {
						case 1:
							tt = stoi(buf.dat[1]);
							tt = 1 << tt;
							tn = cpu.data_p();
							cpu.sys_malloc(((tn - 1) / tt + 1) * tt - tn);
							break;
						case 2: case 3:
							while (n_label.size()) {
								tmp = n_label[n_label.size() - 1];
								data_label[tmp] = cpu.data_p();
								n_label.pop_back();
							}
							tmp2 = "";
							i = 0;
							while (line[i] != '\"') i++;
							i++;
							while (line[i] != '\"') {
								if (line[i] == '\\') {
									switch (line[i + 1]){
										case '\"': tmp2 += '\"'; break;
										case '\'': tmp2 += '\''; break;
										case 'n': tmp2 += '\n'; break;
										case '0': tmp2 += '\0'; break;
									}
									i++;
								}
								else tmp2 += line[i];
								i++;
							}
							if (typ == 3) tmp2 += '\0';
							tt = tmp2.length();
							tn = cpu.data_p();
							for(i = 0; i < tt; i++) cpu.sb(tmp2[i], tn + i);
							cpu.sys_malloc(tt);
							break;
						case 4: case 5: case 6:
							while (n_label.size()) {
								tmp = n_label[n_label.size() - 1];
								data_label[tmp] = cpu.data_p();
								n_label.pop_back();
							}
							tt = buf.dat.size() - 1;
							tn = cpu.data_p();
							for (i = 1; i <= tt; i++) {
								if (typ == 4) {
									cpu.sb(stoi(buf.dat[i]), tn);
									cpu.sys_malloc(1);
								}
								else if (typ == 5) {
									cpu.sh(stoi(buf.dat[i]), tn);
									cpu.sys_malloc(2);
								}
								else {
									cpu.sw(stoi(buf.dat[i]), tn);
									cpu.sys_malloc(4);
								}
							}
							break;
						case 7:
							cpu.sys_malloc(stoi(buf.dat[1]));
							break;
					}

				}
			}
			else {
				while (n_label.size()) {
					tmp = n_label[n_label.size() - 1];
					text_label[tmp] = cpu.text_p();
					n_label.pop_back();
				}
				code tcode(buf);
				cpu.add_text(tcode);
				//cout << cpu.text_p() - 1 << "\t" << line << endl;
			}
		}
		for (int i = 0; i < cpu.text_p(); i++) {
			code tcode = cpu.fetch_ins(i);
			if ((tcode.opr_type >= 26 && tcode.opr_type <= 39)|| tcode.opr_type == 41) {
				tcode.arg[2] = text_label[tcode.label];
				cpu.modi_text(tcode, i);
			}
			else if (tcode.opr_type >= 43 && tcode.opr_type <= 49){
				if (tcode.label != "") {
					tcode.arg[1] = -data_label[tcode.label] - 1;
					cpu.modi_text(tcode, i);
				}
			}
		}
		pip.init(cpu, text_label["main"]);
	}

	int run() {
		pip.run();
		return 0;
	}

private:


};

#endif 
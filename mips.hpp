#ifndef MIPS_HPP
#define MIPS_HPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include "tokenscanner.hpp"
#include "code.hpp"
using namespace std;

const int MXC = 256;

class mips {
	vector<code> text;
	map<string, int> data_label;
	map<string, int> text_label;
	int reg[35], data_buf;
	char data[4 * 1024 * 1024];

public:

	void init(const char* filename) {
		freopen(filename, "r", stdin);
		text.clear();
		string tmp;
		char line[1000];
		tmp = "";
		data_buf = 0;
		memset(reg, 0, sizeof(reg));
		memset(data, 0, sizeof(data));
		codeinit();
		bool is_data = false;
		Tokenscanner buf;
		while (!cin.eof()) {
			gets(line);
			buf.process(line);
			if (buf.dat[0] == ".data") is_data = true;
			else if (buf.dat[0] == ".text") is_data = false;
			else if (is_data) {
				if (buf.dat[0][buf.dat[0].length() - 1] == ':') {
					tmp = buf.dat[0]; tmp.pop_back();
				}
				else {
					int typ = dat_typ[buf.dat[0]];
					int tt, tn, i, j;
					switch (typ) {
						case 1:
							tt = stoi(buf.dat[1]);
							data_buf = ((data_buf - 1) / (1 << tt) + 1) * (1 << tt);
							break;
						case 2:
							if (tmp != "") {
								data_label[tmp] = data_buf;
								tmp = "";
							}
							tt = buf.dat[1].length();
							for(i = 0; i < tt; i++) data[data_buf + i] = buf.dat[1][i];
							data_buf += tt;
							break;
						case 3:
							if (tmp != "") {
								data_label[tmp] = data_buf;
								tmp = "";
							}
							tt = buf.dat[1].length();
							for(i = 0; i < tt; i++) data[data_buf + i] = buf.dat[1][i];
							data_buf += tt;
							data[data_buf] = 0;
							data_buf++;
							break;
						case 4:
							if (tmp != "") {
								data_label[tmp] = data_buf;
								tmp = "";
							}
							tt = buf.dat.size() - 1;
							for (i = 1; i <= tt; i++) {
								data[data_buf + i - 1] = stoi(buf.dat[i]);
							}
							data_buf += tt;
							break;
						case 5:
							if (tmp != "") {
								data_label[tmp] = data_buf;
								tmp = "";
							}
							tt = buf.dat.size() - 1;
							for (i = 1; i <= tt; i++) {
								tn = stoi(buf.dat[i]);
								for (j = 1; j >= 0; j--) {
									data[data_buf + (i - 1) * 2 + j] = tn % MXC;
									tn /= MXC;
								}
							}
							data_buf += tt * 2;
							break;
						case 6:
							if (tmp != "") {
								data_label[tmp] = data_buf;
								tmp = "";
							}
							tt = buf.dat.size() - 1;
							for (i = 1; i <= tt; i++) {
								tn = stoi(buf.dat[i]);
								for (j = 3; j >= 0; j--) {
									data[data_buf + (i - 1) * 4 + j] = tn % MXC;
									tn /= MXC;
								}
							}
							data_buf += tt * 4;
							break;
						case 7:
							data_buf += stoi(buf.dat[1]);
							break;
					}

				}
			}
			else {
				if (tmp != "") {
					text_label[tmp] = text.size();
					tmp = "";
				}
				code tcode(buf);
				text.push_back(tcode);
			}
		}
	}

private:


};

#endif 
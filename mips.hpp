#ifndef MIPS_HPP
#define MIPS_HPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include "tokenscanner.hpp"
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
		code.clear();
		string tmp;
		char line[1000];
		tmp = "";
		data_buf = 0;
		memset(reg, 0, sizeof(reg));
		memset(data, 0, sizeof(data));
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
					switch (buf.dat[0]) {
						case ".align":
							int w = stoi(buf.dat[1]);
							data_buf = ((data_buf - 1) / (1 << w) + 1) * (1 << w);
							break;
						case ".ascii":
							if (tmp != "") {
								data_label[tmp] = data_buf;
								tmp = "";
							}
							int len = buf.dat[1].length();
							for(int i = 0; i < len; i++) data[data_buf + i] = buf.dat[1][i];
							data_buf += len;
							break;
						case "asciiz":
							if (tmp != "") {
								data_label[tmp] = data_buf;
								tmp = "";
							}
							int len = buf.dat[1].length();
							for(int i = 0; i < len; i++) data[data_buf + i] = buf.dat[1][i];
							data_buf += len;
							data[data_buf] = 0;
							data_buf++;
							break;
						case ".byte":
							if (tmp != "") {
								data_label[tmp] = data_buf;
								tmp = "";
							}
							int num = buf.dat.size() - 1;
							for (int i = 1; i <= num; i++) {
								data[data_buf + i - 1] = stoi(buf.dat[i]);
							}
							data_buf += num;
							break;
						case ".half":
							if (tmp != "") {
								data_label[tmp] = data_buf;
								tmp = "";
							}
							int num = buf.dat.size() - 1;
							for (int i = 1; i <= num; i++) {
								int tn = stoi(buf.dat[i]);
								for (int j = 1; j >= 0; j--) {
									data[data_buf + (i - 1) * 2 + j] = tn % MXC;
									tn /= MXC;
								}
							}
							data_buf += num * 2;
							break;
						case ".word":
							if (tmp != "") {
								data_label[tmp] = data_buf;
								tmp = "";
							}
							int num = buf.dat.size() - 1;
							for (int i = 1; i <= num; i++) {
								int tn = stoi(buf.dat[i]);
								for (int j = 3; j >= 0; j--) {
									data[data_buf + (i - 1) * 4 + j] = tn % MXC;
									tn /= MXC;
								}
							}
							data_buf += num * 4;
							break;
						case ".space":
							data_buff += stoi(buf.dat[1]);
							break;
					}

				}
			}
			else {
				if (tmp != "") {
					text_label = text.size();
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
#ifndef token_s
#define token_s

#include <string>
#include <vector>
using namespace std;

class Tokenscanner{
public:
	vector<string> dat;

	void process(const char *st) {
		int len = strlen(st);
		dat.clear();
		string tmp; tmp = "";
		int i = 0;
		while (i < len) {
			while ((st[i] == ' ' || st[i] == '\t' || st[i] == '\0' || st[i] == ',' || st[i] == '(' || st[i] == ')') && i < len) i++;
			if (i >= len) break;
			while ((st[i] != ' ' && st[i] != '\t' && st[i] != ',' && st[i] != '(' && st[i] != ')') && i < len) tmp += st[i], i++;
			if (tmp != "") dat.push_back(tmp);
			tmp = "";
		}
	}
};

#endif
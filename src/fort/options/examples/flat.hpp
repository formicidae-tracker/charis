#pragma once

#include <string>
#include <vector>

struct Options {
	int              AnInt   = 1;
	double           ADouble = 2.0;
	std::string      AString = "foo";
	std::vector<int> AVectorOfInt;
};

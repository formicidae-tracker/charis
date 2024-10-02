#pragma once

#include <string>
#include <vector>

struct Options {
	/** long:"integer" short:"I" description:"An Integer" */
	int              AnInt   = 1;
	/** long:"double" short:"d" description:"A double" required:"true"*/
	double           ADouble = 2.0;
	/** long:"string"  description:"A String" required:"false"*/
	std::string      AString = "foo";
	/** long:"repeat"  short:"R" description:"a repeatable" required:"false"*/
	std::vector<int> AVectorOfInt;
};

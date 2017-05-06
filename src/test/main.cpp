// main.cpp

#include <iostream>
#include "jsonex.h"

void TestJsonEx()
{
	Json::Value v;
	std::string s = v.asString();
	std::cout << "Value = " << s << std::endl;
}

int main(int argc, char* argv[])
{
	std::cout << "Begin." << std::endl;

	TestJsonEx();

	std::cout << std::endl << "End. Press enter to exit." << std::endl;
	getchar();
	return 0;
}

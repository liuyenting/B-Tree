#include <stdexcept>
#include <iostream>
#include <string>

#include "ad_database.h"

#ifdef REMOTE
#define FILE_PATH "/tmp2/KDDCup2012/track2/kddcup2012track2.txt"
#elif LOCAL
#define FILE_PATH "~/dat/kddcup2012track2.txt"
#else
#define MANUAL_FILE_PATH
#endif

void get()
{

}

void clicked()
{

}

void impressed()
{

}

void profit()
{

}

int main(int argc, char* argv[])
{
	#ifndef MANUAL_FILE_PATH
	dsa::Database database(FILE_PATH);
	#else
	try
	{
		if(argc < 2)
			throw std::runtime_error("main(): Too few argument.");
		else if(argc > 2)
			throw std::runtime_error("main(): Program only take one file path as argument.");
		else
			dsa:Database database(std::string(argv[1]));
	}
	catch(std::runtime_error &e)
	{
		std::cerr << "Caught a runtime_error exception: " << e.what () << std::endl;
	}
	#endif

	

	return 0;
}
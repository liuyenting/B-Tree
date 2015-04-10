#include <stdexcept>
#include <iostream>
#include <string>

#include <map>

#include "ad_database.h"

#ifdef REMOTE
#define FILE_PATH "/tmp2/KDDCup2012/track2/kddcup2012track2.txt"
#elif LOCAL
#define FILE_PATH "./dat/kddcup2012track2.txt"
//#define FILE_PATH "./dat/demotrack.txt"
#else
#define MANUAL_FILE_PATH
#endif

#define PRINT_SEPARATOR std::cout << "********************" << std::endl;

void get(dsa::Database& database)
{
	#ifdef DEBUG
	std::cout << "get() called." << std::endl;
	#endif

	unsigned int u, a, q;
	unsigned char p, d;
	std::cin >> u >> a >> q >> p >> d;
	#ifdef DEBUG
	std::cout << "Parameters: (u, a, q, p, d) = ("
			  << u << ", "
			  << a << ", "
			  << q << ", "
			  << p << ", "
			  << d << ")" << std::endl;
	#endif 

	PRINT_SEPARATOR
	std::pair<unsigned int, unsigned long> result = dsa::KDD::get(database, u, a, q, p, d);
	std::cout << result.first << " " << result.second << std::endl;
	PRINT_SEPARATOR
}

void clicked(dsa::Database& database)
{
	#ifdef DEBUG
	std::cout << "clicked() called." << std::endl;
	#endif

	unsigned int u;
	std::cin >> u;
	#ifdef DEBUG
	std::cout << "Parameter: (u) = (" << u << ")" << std::endl;
	#endif 

	PRINT_SEPARATOR
	for(const auto& elem : dsa::KDD::clicked(database, u))
		std::cout << elem.first << " " << elem.second << std::endl;
	PRINT_SEPARATOR
}

void impressed(dsa::Database& database)
{
	#ifdef DEBUG
	std::cout << "impressed() called." << std::endl;
	#endif

	unsigned int u1, u2;
	std::cin >> u1 >> u2;
	#ifdef DEBUG
	std::cout << "Parameter: (u1, u2) = (" 
			  << u1 << ", "
			  << u2 << ")" << std::endl;
	#endif

	PRINT_SEPARATOR
	for(const auto& elem : dsa::KDD::impressed(database, u1, u2))
	{
		std::cout << elem.first << std::endl;
		for(const auto& prop : elem.second)
		{
			std::cout << '\t'
					  << prop.get_display_url() << ' '
					  << prop.get_advertiser_id() << ' '
					  << prop.get_keyword_id() << ' '
					  << prop.get_title_id() << ' '
					  << prop.get_description_id()
					  << std::endl;
		}
	}
	PRINT_SEPARATOR
}

void profit(dsa::Database& database)
{
	#ifdef DEBUG
	std::cout << "profit() called." << std::endl;
	#endif

	unsigned int a;
	double ctr;
	std::cin >> a >> ctr;
	#ifdef DEBUG
	std::cout << "Parameter: (a, ctr) = (" 
			  << a << ", "
			  << ctr << ")" << std::endl;
	#endif

	PRINT_SEPARATOR
	PRINT_SEPARATOR
}

typedef void (*FuncPtr)(dsa::Database&);
typedef std::map<std::string, FuncPtr> InstrMap;

void setup_function_lut(std::map<std::string, FuncPtr>& map)
{
	map["get"] = get;
	map["clicked"] = clicked;
	map["impressed"] = impressed;
	map["profit"] = profit;
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
			dsa::Database database(std::string(argv[1]));
	}
	catch(std::runtime_error &e)
	{
		std::cerr << "Caught a runtime_error exception: " << e.what () << std::endl;
	}
	#endif

	// Setup instruction look-up table
	InstrMap instruction_map;
	setup_function_lut(instruction_map);

	std::string instruction;
	bool quit = false;
	do
	{
		std::cin >> instruction;
		auto elem = instruction_map.find(instruction);
		if (elem == instruction_map.end())
		    quit = true;
		else
		{
			try
			{
				(elem->second)(database);
			}
			catch(std::runtime_error &e)
			{
				// Stop the program if run time error is caught.
				quit = true;
				std::cerr << "Caught a runtime_error exception: " << e.what () << std::endl;
			}
		}
	}while(!quit);

	return 0;
}
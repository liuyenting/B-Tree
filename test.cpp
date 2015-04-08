#include <iostream>
#include <string>

#include "ad_database.h"

#ifdef REMOTE
#define FILE_PATH "/tmp2/KDDCup2012/track2/kddcup2012track2.txt"
#elif LOCAL
#define FILE_PATH "kddcup2012track2.txt"
#else
#define FILE_PATH "kdd_sample"
#endif

int main()
{
	dsa::Database database(FILE_PATH);

	return 0;
}
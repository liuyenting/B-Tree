#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

#include "btree_multimap.h"

#ifndef REMOTE
#define FILE_PATH "kddcup2012track2.txt"
#else
#define FILE_PATH "/tmp2/KDDCup2012/track2/kddcup2012track2.txt"
#endif

struct compound_key
{
    int user_id;
    int ad_id;
};

typedef struct compound_key key;
typedef std::streampos data;

// Compare function for compound_key
struct key_comparer
{
    bool operator()(const key& k1, const key& k2) const
    {
        if(k1.user_id != k2.user_id)
            return k1.user_id < k2.user_id;
        else
            return k1.ad_id < k2.ad_id;
    }
};

unsigned int parse_user_id(std::string &str, char delim)
{
    std::stringstream ss(str);
    unsigned long result;

    while(std::getline(ss, str, delim));
    
    std::stoul(str, &result, 10);
    return static_cast<unsigned int>(result);
}

void construct_tree(std::ifstream& ifs, stx::btree_multimap<unsigned int, std::streamoff, std::less<unsigned int> >&map)
{
    std::string new_line;
    
    int counter = 0;
    while(!ifs.eof())
    {
        std::getline(ifs, new_line);
        
        map.insert(std::pair<unsigned int, std::streamoff>(parse_user_id(new_line, '\t'), ifs.tellg()));
        
        counter++;
        /*
        if((counter % 1000000) == 0)
            std::cout << "Item " << counter << " inserted." << std::endl;
        */
        if((counter / 1000000) == 10)
            break;
    }

    std::cout << "Insertion complete!" << std::endl;
}

int main()
{
    std::ifstream ifs(FILE_PATH, std::ifstream::in);
    
    stx::btree_multimap<unsigned int, std::streamoff, std::less<unsigned int> > BTreeMap;
    
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    construct_tree(ifs, BTreeMap);
    end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Elapsed time: " << elapsed_seconds.count() << std::endl;

    return 0;
}

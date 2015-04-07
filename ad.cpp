#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <streambuf>
#include <istream>

#include <unistd.h>
#include <sys/mman.h>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "btree_multimap.h"

#ifndef REMOTE
#define FILE_PATH "kddcup2012track2.txt"
#else
#define FILE_PATH "/tmp2/KDDCup2012/track2/kddcup2012track2.txt"
#endif

struct membuf : std::streambuf
{
    membuf(char const* base, size_t size)
    {
        char *p(const_cast<char*>(base));
        this->setg(p, p, p + size);
    }
};

struct imemstream : virtual membuf, std::istream
{
    imemstream(char const* base, size_t size) : membuf(base, size),
                                                std::istream(static_cast<std::streambuf*>(this))
    {
    }
};

struct compound_key
{
    int user_id;
    int ad_id;
};

/*
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
*/

unsigned int parse_user_id(std::string &str, char delim)
{
    std::stringstream ss(str);
    unsigned long result;

    while(std::getline(ss, str, delim));
    
    std::stoul(str, &result, 10);
    return static_cast<unsigned int>(result);
}

//void construct_tree(std::ifstream& ifs, stx::btree_multimap<unsigned int, std::streamoff, std::less<unsigned int> >&map)
void construct_tree(imemstream& ims, stx::btree_multimap<unsigned int, std::streamoff, std::less<unsigned int> >&map)
{
    std::string new_line;

    std::cout << "Start constructing tree." << std::endl;
    
    int counter = 0;
    while(!ims.eof())
    {
        std::getline(ims, new_line);
        //std::cout << "New line: " << new_line << std::endl;
        
        // Skip blank line
        if(new_line.length() == 0)
            continue;

        map.insert(std::pair<unsigned int, std::streamoff>(parse_user_id(new_line, '\t'), ims.tellg()));
        
        counter++;
        
        if((counter % 1000000) == 0)
            std::cout << "Item " << counter << " inserted." << std::endl;
        
        //if((counter / 1000000) == 10)
        //    break;
        
    }

    std::cout << "Insertion complete!" << std::endl;
}

size_t getFileSize(const std::string& file_path)
{
    struct stat st;
    if(stat(file_path.c_str(), &st) != 0)
        return 0;
    return st.st_size;
}

int main()
{
    int fd = -1;
    void *data = NULL;

    // Open the file to get the handler.
    fd = fileno(fopen(FILE_PATH, "r"));
    if(fd < 0)
        throw "main(): Fail to open the file.";

    size_t fileSize = getFileSize(FILE_PATH);
    // Map the file into memory
    data = mmap((caddr_t)0, fileSize, PROT_READ, MAP_SHARED, fd, 0);
    if(!data)
        throw "main(): Fail to map the file into memory.";
    else
        std::cout << "File mapped." << std::endl;
    
    imemstream ims(reinterpret_cast<char*>(data), fileSize);

    //std::ifstream ifs(FILE_PATH, std::ifstream::in);

    stx::btree_multimap<unsigned int, std::streamoff, std::less<unsigned int> > BTreeMap;
    
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    //construct_tree(ifs, BTreeMap);
    construct_tree(ims, BTreeMap);
    end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Elapsed time: " << elapsed_seconds.count() << std::endl;

    return 0;
}

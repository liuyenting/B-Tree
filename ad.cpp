#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <streambuf>
#include <istream>

#ifdef MMF
#include <unistd.h>
#include <sys/mman.h>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <list>

#include "btree_multimap.h"

#ifdef REMOTE
#define FILE_PATH "/tmp2/KDDCup2012/track2/kddcup2012track2.txt"
#elif LOCAL
#define FILE_PATH "kddcup2012track2.txt"
#else
#define FILE_PATH "kdd_sample"
#endif

#ifdef MMF
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
#endif

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

    while(std::getline(ss, str, delim));

    return static_cast<unsigned int>(std::stoi(str, 0, 10));
}

#ifndef MMF
void construct_tree(std::ifstream& stream, stx::btree_multimap<unsigned int, std::streamoff, std::less<unsigned int> >&map)
#else
void construct_tree(imemstream& stream, stx::btree_multimap<unsigned int, std::streamoff, std::less<unsigned int> >&map)
#endif
{
    std::string new_line;

    std::cout << "Start constructing tree." << std::endl;
    
    int counter = 0;
    std::ifstream::pos_type currentPos = 0;

    stream.clear();
    std::cout << "Init fpos: " << stream.tellg() << std::endl;

    while(!stream.eof())
    {
        // Get the pos_type of from the head of current line
        currentPos = stream.tellg();
        std::getline(stream, new_line);
        
        // Skip blank line
        if(new_line.length() == 0)
            continue;

        map.insert(std::pair<unsigned int, std::streamoff>(parse_user_id(new_line, '\t'), currentPos));
        
        counter++;
        
        if((counter % 1000000) == 0)
            std::cout << "Item " << counter << " inserted." << std::endl;
        
        //if((counter / 1000000) == 10)
        //    break;
        
    }

    std::cout << "Insertion complete!" << std::endl;
}

#ifdef MMF
size_t getFileSize(const std::string& file_path)
{
    struct stat st;
    if(stat(file_path.c_str(), &st) != 0)
        return 0;
    return st.st_size;
}
#endif

int main()
{
    #ifdef MMF
    int fd = -1;
    void *data = NULL;

    // Open the file to get the handler.
    fd = fileno(fopen(FILE_PATH, "r"));
    if(fd < 0)
        throw "main(): Fail to open the file.";

    size_t fileSize = getFileSize(FILE_PATH);
    // Map the file into memory
    data = mmap((caddr_t)0, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    if(data == MAP_FAILED)
        throw "main(): Fail to map the file into memory.";
    else
        std::cout << "File mapped." << std::endl;
    
    imemstream ims(reinterpret_cast<char*>(data), fileSize);
    #else
    std::ifstream ifs(FILE_PATH, std::ifstream::in);
    #endif

    stx::btree_multimap<unsigned int, std::streamoff, std::less<unsigned int> > BTreeMap;
    
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    #ifndef MMF
    construct_tree(ifs, BTreeMap);
    #else
    construct_tree(ims, BTreeMap);
    #endif
    /*
    BTreeMap.insert(std::pair<unsigned int, std::streamoff>(0,0));
    BTreeMap.insert(std::pair<unsigned int, std::streamoff>(0,1));
    BTreeMap.insert(std::pair<unsigned int, std::streamoff>(0,2));
    BTreeMap.insert(std::pair<unsigned int, std::streamoff>(1,3));
    BTreeMap.insert(std::pair<unsigned int, std::streamoff>(2,4));
    BTreeMap.insert(std::pair<unsigned int, std::streamoff>(3,5));
    */
    end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Elapsed time: " << elapsed_seconds.count() << std::endl;

    std::pair<stx::btree_multimap<unsigned int, std::streamoff, std::less<unsigned int> >::iterator, 
              stx::btree_multimap<unsigned int, std::streamoff, std::less<unsigned int> >::iterator> range;
    range = BTreeMap.equal_range(490234);

    std::list<std::streamoff> lst;

    for(stx::btree_multimap<unsigned int, std::streamoff, std::less<unsigned int> >::iterator it = range.first; it != range.second; ++it)
        lst.push_back(it->second);

    // Wipe the position flag
    #ifdef MMF
    ims.clear();
    #else
    ifs.clear();
    #endif

    std::cout << "Found (fpos) {" << std::endl;
    for (auto it = lst.begin(); it != lst.end(); ++it)
    {
        std::string buffer;

        std::cout << "Seeking at: " << *it << std::endl;

        #ifdef MMF
        ims.seekg(*it, ims.beg);
        std::getline(ims, buffer);
        #else
        ifs.seekg(*it, ifs.beg);
        std::cout << "Current fpos: " << ifs.tellg() << std::endl;
        std::getline(ifs, buffer);
        #endif

        std::cout << buffer << std::endl;
    }
    std::cout << " }" << std::endl;

    #ifdef MMF
    // Unmap the file
    if(munmap(data, fileSize) == -1)
        throw "main(): Fail to un-mapping the file.";
    close(fd);
    #endif

    return 0;
}

#ifndef _AD_DATABASE_H
#define _AD_DATABASE_H

// Includes mainly for class Database
#include <stdexcept>
#include <iostream>
#ifndef MMF
#include <fstream>
#else
// Includes especially for memory mapped files
#include <unistd.h>
#include <sys/mman.h>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <type_traits>

// Includes mainly for class Entry
#include <sstream>

// Includes mainly for class KDD
#include <omp.h>
#include <parallel/algorithm>
#include <algorithm>
#include <map>
#include <list>

#include "btree_multimap.h"

// Definitions for field parsing
#define NEWLINE 			'\n'
#define DELIM 				'\t'

// Debug paramters
#ifdef DEBUG
#include <chrono>
#define NOTICE_PER_LINE		1000000
//#define PAUSE_AT_RATIO	10
#endif

// Performance parameters
#define SLOTS 				128
//#define BIN_THRESHOLD 		256*1024*1024
#define BIN_THRESHOLD 		1024

namespace dsa
{	
	typedef unsigned int TKey;
	#ifndef MMF
	typedef std::streamoff TData;
	#else
	typedef size_t TData;
	#endif

	// Trait setup for the map
	template <int _innerSlots, int _leafSlots>
	struct btree_traits_speed : stx::btree_default_set_traits<TKey>
	{
	    static const bool   selfverify = false;
	    static const bool   debug = false;

	    static const int    leafslots = _innerSlots;
	    static const int    innerslots = _leafSlots;

	    static const size_t binsearch_threshold = BIN_THRESHOLD;
	};
	typedef stx::btree_multimap<TKey, TData, 
								std::less<TKey>, 
								struct btree_traits_speed<SLOTS, SLOTS> > BpTreeMap;

	// TSV field definitions
	enum field 
	{
		CLICK = 0, IMPRESSION,
		DISPLAY_URL, AD_ID, ADVERTISER_ID,
		DEPTH, POSITION,
		QUERY_ID, KEYWORD_ID, TITLE_ID, DESCRIPTION_ID,
		USER_ID 
	};

	#ifdef MMF
	class MemoryMappedFile
	{
	private:
		int fd = -1;
		char* data = NULL;
		size_t file_size;

		// Offset in the memory mapped file
		size_t off;

	private:
		// Support function for initialization
		size_t get_file_size(const std::string& file_path)
		{
			struct stat st;
			if(stat(file_path.c_str(), &st) != 0)
				return 0;
			return st.st_size;
		}

	public:
		MemoryMappedFile()
		{
			off = 0;
		}

		void open(const std::string& file_path)
		{
			fd = fileno(fopen(file_path.c_str(), "r"));
        	if(fd < 0)
        		throw std::runtime_error("MMF(): Fail to open the file.");

        	file_size = get_file_size(file_path);
        	#ifdef DEBUG
        	std::cout << "File size: " << file_size << std::endl;
        	#endif

        	// Map the file into memory
        	data = reinterpret_cast<char*>(mmap((caddr_t)0, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
			if(data == MAP_FAILED)
				throw std::runtime_error("MMF(): Fail to map the file into memory.");
			#ifdef DEBUG
			else
				std::cout << "File mapped." << std::endl;
			#endif
		}

	public:
		// Stream support functions
		bool eof() const
		{
			return (off == file_size);
		}

		TData tellg() const
		{
			return off;
		}

		void seekg(const TData& val)
		{
			off = val;
		}

		char& getc()
		{
			return data[off++];
		}

		char* getp()
		{
			return data + off;
		}

		std::string getline()
		{
			size_t length = 0;
			for(size_t pos = off; data[pos] != NEWLINE; pos++, length++);
			return std::string(getp(), length);
		}
	};
	#endif

	class Database
	{
	private:
		#ifdef DEBUG
		// Variable for timing
	    std::chrono::time_point<std::chrono::system_clock> start, end;
        std::chrono::duration<double> elapsed_seconds;
        #endif
        #ifndef MMF
        std::ifstream stream;
        #else
        MemoryMappedFile mmf;
        #endif
        BpTreeMap map, ad_id_map;

	public:
		Database(const std::string& file_path)
		{
			#ifndef MMF
			stream.open(file_path, std::ifstream::in);
			if(!stream.is_open())
				throw std::runtime_error("Database(): Fail to open file.");
			#else
			mmf.open(file_path);
			#endif
			construct_tree();
		}

		~Database()
		{
		}

		#ifndef MMF
		std::ifstream& get_stream()
		{
			return stream;
		}
		#endif

		void construct_tree()
		{
			std::string new_line;

			#ifdef DEBUG
			std::cout << "Start constructing tree..." << std::endl;
			
			// Counter for cycles
			int counter = 0;
			#endif

			TData currentPos = 0;
			#ifdef DEBUG
			// Start timer
			start = std::chrono::system_clock::now();
			#endif
			#ifndef MMF
			while(!stream.eof())
			#else
			while(!mmf.eof())
			#endif
			{
				// Get the position of current line
				#ifndef MMF
				currentPos = stream.tellg();
				std::getline(stream, new_line);
				#else
				//std::cout << "Getting current fpos..." << std::endl;
				currentPos = mmf.tellg();
				//std::cout << "Current fpos: " << currentPos << std::endl;
				#endif

				#ifndef MMF
				// Skip blank line
				if(new_line.length() == 0)
					continue;

				map.insert(std::make_pair(parse_field<TKey, USER_ID>(new_line, DELIM), currentPos));
				#else
				map.insert(std::make_pair(parse_field<TKey, USER_ID>(mmf, DELIM), currentPos));
				mmf.seekg(currentPos);
				ad_id_map.insert(std::make_pair(parse_field<TKey, AD_ID>(mmf, DELIM), currentPos));
				#endif

				#ifdef DEBUG
				counter++;
				if((counter % NOTICE_PER_LINE) == 0)
					std::cout << "Item " << counter << " inserted." << std::endl;
				#ifdef PAUSE_AT_RATIO
				if((counter / NOTICE_PER_LINE) == PAUSE_AT_RATIO)
					break;
				#endif
				#endif
			}
			#ifdef DEBUG
			// End timer
			end = std::chrono::system_clock::now();
			std::cout << "Insertion complete! ";
			elapsed_seconds = end - start;
			std::cout << "Elapsed time: " << elapsed_seconds.count() << std::endl;
			#endif
		}

		template <typename FieldType, enum field Field>
		#ifndef MMF
		FieldType parse_field(std::string &str, const char& delim)
		#else
		FieldType parse_field(MemoryMappedFile& mmf, const char& delim)
		#endif		
		{
			static_assert(std::is_same<FieldType, unsigned char>::value ||
						  std::is_same<FieldType, unsigned short>::value ||
						  std::is_same<FieldType, unsigned int>::value || 
						  std::is_same<FieldType, unsigned long long>::value,
						  "parse_field(): Designated template type isn't acceptable.");

			FieldType result = 0;
			int ptr = 0;
			char c = mmf.getc();

			#ifndef MMF
			// Shift to desired field according to deliminator.
			for(int idx = 0; idx < Field; ptr++)
			{
				if(str[ptr] == delim)
					idx++;
				if(str[ptr] == NEWLINE)
					throw std::runtime_error("parse_field(): Field out of range.");
			}
			#else
			for(int idx = 0; idx < Field; c = mmf.getc())
			{
				if(c == delim)
					idx++;
				if(c == NEWLINE)
					throw std::runtime_error("parse_field(): Field out of range.");
			}
			#endif

			// Start extracting the number.
			// Stop when: deliminator, newline character, end-of-string, is found.
			#ifndef MMF
			for(; str[ptr] != delim && 
				  str[ptr] != NEWLINE && 
				  str[ptr] != '\0'; ptr++)
			{
				result *= 10;
				result += str[ptr] - '0';
			}
			#else
			for(; c != delim && 
				  c != NEWLINE && 
				  c != '\0'; c = mmf.getc())
			{
				result *= 10;
				result += c - '0';
			}
			#endif

			return result;
		}

		// Friend classes
		friend class KDD;
	};

	class Entry
	{
	private:
		unsigned short click;
		unsigned int impression;
		unsigned long long display_url;
		unsigned int ad_id;
		unsigned short advertiser_id;
		unsigned char depth;
		unsigned char position;
		unsigned int query_id;
		unsigned int keyword_id;
		unsigned int title_id;
		unsigned int description_id;
		unsigned int user_id;

	public:
		// Getters
		// getters for get()
		unsigned short get_click() const
		{
			return click;
		}

		unsigned int get_impression() const
		{
			return impression;
		}

		// getters for clicked()
		unsigned int get_ad_id() const
		{
			return ad_id;
		}

		unsigned int get_query_id() const
		{
			return query_id;
		}

		// getters for impression()
		unsigned long long get_display_url() const
		{
			return display_url;
		}
		
		unsigned short get_advertiser_id() const
		{
			return advertiser_id;
		}

		unsigned int get_keyword_id() const
		{
			return keyword_id;
		}

		unsigned int get_title_id() const
		{
			return title_id;
		}

		unsigned int get_description_id() const
		{
			return description_id;
		}

		unsigned int get_user_id() const
		{
			return user_id;
		}

	public:
		// TODO: Direct extraction
		Entry(const std::string& entry)
		{
			std::istringstream iss(entry);
			iss >> click >> impression 
			    >> display_url >> ad_id >> advertiser_id
			    >> depth >> position
			    >> query_id >> keyword_id >> title_id
			    >> description_id
			    >> user_id;
		}

		// Functions for filtering
		bool isGet(unsigned int _user_id,
				   unsigned int _ad_id,
				   unsigned int _query_id,
				   unsigned char _position,
				   unsigned char _depth) const
		{
			return (_user_id == user_id) &&
				   (_ad_id == ad_id) &&
				   (_query_id == query_id) &&
				   (_position == position) &&
				   (_depth == depth);
		}

		bool hasClicked() const
		{
			return click > 0;
		}

		bool hasImpression() const
		{
			return impression > 0;
		}
	};

	class KDD
	{
	//
	// get()
	//
	private:
		static std::vector<Entry> _filter_by_user_id_wrapper(Database& database, unsigned int _user_id)
		{
			#ifndef MMF
			// Reset the stream
			database.stream.clear();
			#endif

			// Search in the database
			auto range = database.map.equal_range(_user_id);

			// Start conversion
			std::string tmp;
			std::vector<Entry> result;
			/*
			for(auto it = range.first; it != range.second; ++it)
			{
				database.stream.seekg(it->second, database.stream.beg);
				std::getline(database.stream, tmp);

				#pragma omp critical
				result.push_back(Entry(tmp));
			}
			*/
			/*
			__gnu_parallel::for_each(range.first, range.second, 
									 [&database, &tmp, &result](std::pair<TKey, TData> &it)
									    { 
									  		database.stream.seekg(it.second, database.stream.beg);
											std::getline(database.stream, tmp);

											#pragma omp critical
											result.push_back(Entry(tmp));	
									  	});
			*/
			
			#pragma omp parallel
			{
			    std::vector<Entry> result_private;
			    for(auto it = range.first; it != range.second; ++it)
				#pragma omp single nowait
				{
					database.mmf.seekg(it->second);
					result_private.push_back(Entry(database.mmf.getline()));
				}

			    #pragma omp critical
			    result.insert(result.end(), result_private.begin(), result_private.end());
			}
			
			return result;
		}

	public:
		// Output the sum of click and impression.
		static std::pair<unsigned int, unsigned long> get(Database& database,
														  unsigned int _user_id,
												   		  unsigned int _ad_id, unsigned int _query_id,
												   		  unsigned char _position, unsigned char _depth)
		{
			unsigned int clicks = 0;
			unsigned long impression = 0;

			for(const auto& elem : _filter_by_user_id_wrapper(database, _user_id))
			{
				if(elem.isGet(_user_id, _ad_id, _query_id, _position, _depth))
				{
					clicks += elem.get_click();
					impression += elem.get_impression();
				}
			}

			return std::make_pair(clicks, impression);
		}

	//
	// clicked()
	//
	public:
		static std::list<std::pair<unsigned int, unsigned int> > clicked(Database& database, unsigned int _user_id)
		{
			std::list<std::pair<unsigned int, unsigned int> > lst;
			for(const auto& elem : _filter_by_user_id_wrapper(database, _user_id))
			{
				if(elem.hasClicked())
					lst.push_back(std::make_pair(elem.get_ad_id(), elem.get_query_id()));
			}

			// Sort in ascending mode and then remove duplicate entries.
			lst.sort();
			lst.unique();

			return lst;
		}

	//
	// impressed()
	//
	private:
		static bool ad_id_list_comparer(Entry const& lhs, Entry const& rhs)
	    {
	    	return lhs.get_ad_id() < rhs.get_ad_id();
	    }

	public:
		static std::map<unsigned int, std::vector<Entry> > impressed(Database& database,
																   unsigned int _user_id_1, unsigned int _user_id_2)
		{
			// Acquire the intersected ads between both users
			std::vector<Entry> user_1 = _filter_by_user_id_wrapper(database, _user_id_1);
			std::cout << "user_1 filtered" << std::endl;
			std::vector<Entry> user_2 = _filter_by_user_id_wrapper(database, _user_id_2);
			std::cout << "user_2 filtered" << std::endl;
			std::vector<Entry> intersection;
			// Sort the list
			//user_1.sort(ad_id_list_comparer);
			__gnu_parallel::sort(user_1.begin(), user_1.end(), ad_id_list_comparer);
			std::cout << "user_1 sorted" << std::endl;
			//user_2.sort(ad_id_list_comparer);
			__gnu_parallel::sort(user_2.begin(), user_2.end(), ad_id_list_comparer);
			std::cout << "user_2 sorted" << std::endl;
			// Find the intersected ads between user1 and user2
			std::set_intersection(user_1.begin(), user_1.end(),
								  user_2.begin(), user_2.end(),
								  std::back_inserter(intersection),
								  [](Entry const& lhs, Entry const& rhs) 
								    { 
								  		if(!lhs.hasImpression() || !rhs.hasImpression())
								  	    	return false;
								  	  	else
								      		return lhs.get_ad_id() == rhs.get_ad_id(); 
								    } );
			std::cout << "intersection found" << std::endl;
			// Refine the result for map
			std::map<unsigned int, std::vector<Entry> > map;
			for(const auto& elem : intersection)
			{
				// Append the property into vector if exists
				if(map.count(elem.get_ad_id()))
					map[elem.get_ad_id()].push_back(elem);
				else
				{
					std::vector<Entry> tmp_lst;
					tmp_lst.push_back(elem);
					map.insert(std::make_pair(elem.get_ad_id(), tmp_lst));
				}
			}

			return map;
		}

	//
	// profit()
	//	
	public:
		static std::vector<TKey> profit(Database& database,
									  unsigned int _ad_id, double _ctr_threshold)
		{
			std::vector<TKey> lst;

			#ifdef DEBUG
			// Variable for timing
		    std::chrono::time_point<std::chrono::system_clock> start, end;
	        std::chrono::duration<double> elapsed_seconds;

			// Start timer
			start = std::chrono::system_clock::now();
			#endif
			#pragma omp parallel
			{
				std::vector<TKey> lst_private;
				for(BpTreeMap::iterator it = database.map.begin();
					it != database.map.end(); 
					it = database.map.upper_bound(it->first))
				#pragma omp single nowait
				{
					double ctr = 0.0;

					TKey tmp = it->first;

					for(const auto& elem : _filter_by_user_id_wrapper(database, tmp))
					{
						if(elem.get_ad_id() == _ad_id)
						{
							ctr += (double)elem.get_click() / elem.get_impression();
							if(ctr >= _ctr_threshold)
							{
								lst_private.push_back(tmp);
								break;
							}
						}
					}

					/*
					if((clicks == 0) && (impression == 0))
					{
						if(_ctr_threshold == 0)
							lst_private.push_back(tmp);
					}
					else if((impression != 0) && ((double)clicks / impression) >= _ctr_threshold)
						lst_private.push_back(tmp);
					*/
				}

				#pragma omp critical
				lst.insert(lst.end(), lst_private.begin(), lst_private.end());
			}
			#ifdef DEBUG
			// End timer
			end = std::chrono::system_clock::now();
			std::cout << "Search complete! ";
			elapsed_seconds = end - start;
			std::cout << "Elapsed time: " << elapsed_seconds.count() << std::endl;
			#endif

			__gnu_parallel::sort(lst.begin(), lst.end());

			return lst;
		}
	};
}

#endif
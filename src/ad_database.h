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

		~MemoryMappedFile()
		{
			if(munmap(data, file_size) == -1)
		        throw "main(): Fail to un-mapping the file.";
		    close(fd);
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
        #ifndef MMF
        std::ifstream stream;
        #else
        MemoryMappedFile mmf;
        #endif
        BpTreeMap map, ad_id_map;
        stx::btree_multimap<TKey, TKey, 
							std::less<TKey>, 
							struct btree_traits_speed<SLOTS, SLOTS> > user_id_ad_id_map;

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

			#ifdef DEBUG
			std::cout << "Start constructing tree..." << std::endl;
			// Counter for cycles
			int counter = 0;
			#endif

			TData currentPos = 0;
			//#pragma omp parallel 
			//{
			//#pragma omp single
			//{
			#ifndef MMF
			std::string new_line;
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

				auto user = parse_field<TKey, USER_ID>(mmf, DELIM);
				mmf.seekg(currentPos);
				auto ad = parse_field<TKey, AD_ID>(mmf, DELIM);
				//#pragma omp parallel
				//{
				//#pragma omp single nowait
				//{
					//#pragma omp task
					map.insert(std::make_pair(user, currentPos));
					//#pragma omp task
					ad_id_map.insert(std::make_pair(ad, currentPos));
					//#pragma omp task
					user_id_ad_id_map.insert(std::make_pair(user, ad));
				//}
				//#pragma omp taskwait
				//}

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
			//}
			//}
			#ifdef DEBUG
			std::cout << "... Complete!" << std::endl;
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
			#ifndef MMF
			int ptr = 0;
			#else
			char c = mmf.getc();
			#endif

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
				//std::cout << "c=" << c << ", idx=" << idx << std::endl;
				if(c == delim)
					idx++;
				if(c == NEWLINE)
					throw std::runtime_error("parse_field(): Field out of range.");
			}
			//std::cout << std::endl;
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

			// Fast forward to end of the string
			for( ; c != NEWLINE; c = mmf.getc());
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
		Entry()
		{
		}

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

			std::vector<TData> list;
			for(auto it = range.first; it != range.second; ++it)
				list.push_back(it->second);

			// Start conversion
			/*
			int size = list.size();
			std::vector<Entry> result(size);
			#pragma omp parallel for shared(result, database, list)
			for(int idx = 0; idx < size; ++idx) 
			{
			    database.mmf.seekg(list[idx]);
				result[idx] = Entry(database.mmf.getline());
			}
			*/

			std::vector<Entry> result;
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
			
			/*
			__gnu_parallel::for_each(range.first, range.second, 
									 [&result, &database](std::pair<TKey, TData>& elem)
									 {
										database.mmf.seekg(elem.second);
										//#pragma omp critical
										result.push_back(Entry(database.mmf.getline()));
									 });
			*/

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
		static std::vector<std::pair<unsigned int, unsigned int> > clicked(Database& database, unsigned int _user_id)
		{
			std::vector<std::pair<unsigned int, unsigned int> > result;
			for(const auto& elem : _filter_by_user_id_wrapper(database, _user_id))
			{
				if(elem.hasClicked())
					result.push_back(std::make_pair(elem.get_ad_id(), elem.get_query_id()));
			}

			// Sort in ascending mode and then remove duplicate entries.
			__gnu_parallel::sort(result.begin(), result.end());
			result.erase(std::unique(result.begin(), result.end()), result.end());
			
			return result;
		}

	//
	// impressed()
	//
	private:
		static bool ad_id_list_comparer(Entry const& lhs, Entry const& rhs)
	    {
	    	return lhs.get_ad_id() < rhs.get_ad_id();
	    }

		template <typename Pair>
		struct Equal : public std::binary_function<Pair, Pair, bool>
		{
		    bool operator()(const Pair &x, const Pair &y) const
		    {
		        return x.first == y.first;
		    }
		};

	public:
		static std::map<unsigned int, std::vector<Entry> > impressed(Database& database,
																   unsigned int _user_id_1, unsigned int _user_id_2)
		{
			// Dummy map
			std::map<unsigned int, std::vector<Entry> > result;

			#ifdef DEBUG
			std::cout << "start searching ads viewed by user 1...";
			#endif
			// Search the ad from user 1
			auto range1 = database.user_id_ad_id_map.equal_range(_user_id_1);
			std::vector<TKey> user_1_ad_list;
			for(auto it = range1.first; it != range1.second; it++)
				user_1_ad_list.push_back(it->second);
			__gnu_parallel::sort(user_1_ad_list.begin(), user_1_ad_list.end());
			#ifdef DEBUG
			std::cout << "complete" << std::endl;
			#endif

			#ifdef DEBUG
			std::cout << "start searching ads viewed by user 2...";
			#endif
			// Search the ad from user 2
			auto range2 = database.user_id_ad_id_map.equal_range(_user_id_2);
			std::vector<TKey> user_2_ad_list;
			for(auto it = range2.first; it != range2.second; it++)
				user_2_ad_list.push_back(it->second);
			__gnu_parallel::sort(user_2_ad_list.begin(), user_2_ad_list.end());
			#ifdef DEBUG
			std::cout << "complete" << std::endl;
			#endif

			#ifdef DEBUG
			std::cout << "start finding intersected ads between both users...";
			#endif
			// Find the intersected ads between user 1 and user 2
			std::vector<TKey> intersected_ads;
			__gnu_parallel::set_intersection(user_1_ad_list.begin(), user_1_ad_list.end(),
                          					 user_2_ad_list.begin(), user_2_ad_list.end(),
                          					 std::back_inserter(intersected_ads));
			intersected_ads.erase(std::unique(intersected_ads.begin(), intersected_ads.end()), intersected_ads.end());
			#ifdef DEBUG
			std::cout << "complete" << std::endl;
			#endif

			#ifdef DEBUG
			std::cout << "start searching for properties..." << std::endl;
			#endif
			for(const auto& elem : intersected_ads)
			{
				std::vector<Entry> tmp_vec;

				auto range = database.ad_id_map.equal_range(elem);
				for(auto it = range.first; it != range.second; ++it)
				{
					database.mmf.seekg(it->second);
					Entry tmp(database.mmf.getline());

					if((tmp.get_user_id() == _user_id_1) || (tmp.get_user_id() == _user_id_2))
					{
						if(tmp.hasImpression())
							tmp_vec.push_back(tmp);
					}
				}

				// Sort and remove duplicate entries
				__gnu_parallel::sort(tmp_vec.begin(), tmp_vec.end(), 
									 [](const Entry&lhs, const Entry& rhs)
									 	{
									 		if(lhs.get_advertiser_id() != rhs.get_advertiser_id())
									 			return lhs.get_advertiser_id() < rhs.get_advertiser_id();
									 		else if(lhs.get_keyword_id() != rhs.get_keyword_id())
									 			return lhs.get_keyword_id() < rhs.get_keyword_id();
									 		else if(lhs.get_title_id() != rhs.get_title_id())
									 			return lhs.get_title_id() < rhs.get_title_id();
									 		else
									 			return lhs.get_description_id() < rhs.get_description_id();
									 	});
				tmp_vec.erase(std::unique(tmp_vec.begin(), tmp_vec.end(),
										   [](const Entry& lhs, const Entry& rhs)
										   	{
										   		return (lhs.get_advertiser_id() == rhs.get_advertiser_id()) &&
										   			   (lhs.get_keyword_id() == rhs.get_keyword_id()) &&
										   			   (lhs.get_title_id() == rhs.get_title_id()) &&
										   			   (lhs.get_description_id() == rhs.get_description_id());
										    }), tmp_vec.end());

				result[elem] = tmp_vec;
			}
			#ifdef DEBUG
			std::cout << "...complete" << std::endl;
			#endif

			// Start conversion
			/*
			int size = list.size();
			std::vector<Entry> result(size);
			#pragma omp parallel for shared(result, database, list)
			for(int idx = 0; idx < size; ++idx) 
			{
			    database.mmf.seekg(list[idx]);
				result[idx] = Entry(database.mmf.getline());
			}
			*/


			/*
			// Acquire the intersected ads between both users
			std::vector<Entry> user_1 = _filter_by_user_id_wrapper(database, _user_id_1);
			#ifdef DEBUG
			std::cout << "User 1 filtered" << std::endl;
			#endif
			std::vector<Entry> user_2 = _filter_by_user_id_wrapper(database, _user_id_2);
			#ifdef DEBUG
			std::cout << "User 2 filtered" << std::endl;
			#endif
			std::vector<Entry> intersection;
			*/

			// Sort the list
			//user_1.sort(ad_id_list_comparer);
			/*
			__gnu_parallel::sort(user_1.begin(), user_1.end(), ad_id_list_comparer);
			std::cout << "user_1 sorted" << std::endl;
			//user_2.sort(ad_id_list_comparer);
			__gnu_parallel::sort(user_2.begin(), user_2.end(), ad_id_list_comparer);
			std::cout << "user_2 sorted" << std::endl;
			*/
			// Find the intersected ads between user1 and user2

			/*
			#pragma omp parallel 
			{
				std::vector<Entry> intersection_private;
				
				for(auto a = user_1.begin(); a != user_1.end(); ++a)
				#pragma omp single nowait
				{
					for(auto b = user_2.begin(); b != user_2.end(); ++b)
					{
						if(a->hasImpression() || b->hasImpression())
						{
							if(a->get_ad_id() == b->get_ad_id())
							{
								intersection_private.push_back(*a);
								if((a->get_advertiser_id() != b->get_advertiser_id()) ||
								   (a->get_keyword_id() != b->get_keyword_id()) ||
								   (a->get_title_id() != b->get_title_id()) ||
								   (a->get_description_id() != b->get_description_id()))
								{
									intersection_private.push_back(*b);
								}
							}
						}
					}
				}

				#pragma omp critical
			    intersection.insert(intersection.end(), intersection_private.begin(), intersection_private.end());
			}
			
			__gnu_parallel::sort(intersection.begin(), intersection.end(), 
								 [](const Entry&lhs, const Entry& rhs)
								 	{
								 		if(lhs.get_advertiser_id() != rhs.get_advertiser_id())
								 			return lhs.get_advertiser_id() < rhs.get_advertiser_id();
								 		else if(lhs.get_keyword_id() != rhs.get_keyword_id())
								 			return lhs.get_keyword_id() < rhs.get_keyword_id();
								 		else if(lhs.get_title_id() != rhs.get_title_id())
								 			return lhs.get_title_id() < rhs.get_title_id();
								 		else
								 			return lhs.get_description_id() < rhs.get_description_id();
								 	});
			intersection.erase(std::unique(intersection.begin(), intersection.end(),
										   [](const Entry& lhs, const Entry& rhs)
										   	{
										   		return (lhs.get_advertiser_id() == rhs.get_advertiser_id()) &&
										   			   (lhs.get_keyword_id() == rhs.get_keyword_id()) &&
										   			   (lhs.get_title_id() == rhs.get_title_id()) &&
										   			   (lhs.get_description_id() == rhs.get_description_id());
										    }), intersection.end());
			#ifdef DEBUG
			std::cout << "Intersections between u1 and u2 found." << std::endl << std::endl;
			#endif

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
			*/
			
			return result;
		}

	//
	// profit()
	//	
	public:
		static std::vector<TKey> profit(Database& database,
									  unsigned int _ad_id, double _ctr_threshold)
		{
			std::vector<TKey> lst;

			/*
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
				}

				#pragma omp critical
				lst.insert(lst.end(), lst_private.begin(), lst_private.end());
			}
			*/

			// Search in the database
			auto range = database.ad_id_map.equal_range(_ad_id);
			
			std::map<unsigned int, double> record;
			for(auto it = range.first; it != range.second; ++it)
			{
				database.mmf.seekg(it->second);
				Entry tmp(database.mmf.getline());

				if(record.count(tmp.get_user_id()))
					record[tmp.get_user_id()] += (double)tmp.get_click() / tmp.get_impression();
				else
					record.insert(std::make_pair(tmp.get_user_id(), (double)tmp.get_click() / tmp.get_impression()));
			}

			for(const auto& elem : record)
			{
				if(elem.second > _ctr_threshold)
					lst.push_back(elem.first);
			}

			__gnu_parallel::sort(lst.begin(), lst.end());

			return lst;
		}
	};
}

#endif
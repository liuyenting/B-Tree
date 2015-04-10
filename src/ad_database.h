#ifndef _AD_DATABASE_H
#define _AD_DATABASE_H

// Includes mainly for class Database
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <type_traits>

// Includes mainly for class Entry
#include <sstream>

// Includes mainly for class KDD
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
#define BIN_THRESHOLD 		256*1024*1024

namespace dsa
{	
	typedef unsigned int TKey;
	typedef std::streamoff TData;

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

	class Database
	{
	private:
		#ifdef DEBUG
		// Variable for timing
	    std::chrono::time_point<std::chrono::system_clock> start, end;
        std::chrono::duration<double> elapsed_seconds;
        #endif
        std::ifstream stream;
        BpTreeMap map;

	public:
		Database(const std::string& file_path)
		{
			stream.open(file_path, std::ifstream::in);
			if(!stream.is_open())
				throw std::runtime_error("Database(): Fail to open file.");

			construct_tree(stream, map);
		}

		~Database()
		{
		}

		std::ifstream& get_stream()
		{
			return stream;
		}

		void construct_tree(std::ifstream& stream, BpTreeMap& map)
		{
			std::string new_line;

			#ifdef DEBUG
			std::cout << "Start constructing tree..." << std::endl;
			
			// Counter for cycles
			int counter = 0;

			// Variable for timing
			#endif

			TData currentPos = 0;
			#ifdef DEBUG
			// Start timer
			start = std::chrono::system_clock::now();
			#endif
			while(!stream.eof())
			{
				// Get the position of current line
				currentPos = stream.tellg();
				std::getline(stream, new_line);

				// Skip blank line
				if(new_line.length() == 0)
					continue;

				map.insert(std::pair<TKey, TData>(parse_field<TKey, USER_ID>(new_line, DELIM), 
												  currentPos));

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
		FieldType parse_field(std::string &str, const char& delim)
		{
			static_assert(std::is_same<FieldType, unsigned char>::value ||
						  std::is_same<FieldType, unsigned short>::value ||
						  std::is_same<FieldType, unsigned int>::value || 
						  std::is_same<FieldType, unsigned long long>::value,
						  "parse_field(): Designated template type isn't acceptable.");

			FieldType result = 0;
			int ptr = 0;
			
			// Shift to desired field according to deliminator.
			for(int idx = 0; idx < Field; ptr++)
			{
				if(str[ptr] == delim)
					idx++;
				if(str[ptr] == NEWLINE)
					throw std::runtime_error("parse_field(): Field out of range.");
			}

			// Start extracting the number.
			// Stop when: deliminator, newline character, end-of-string, is found.
			for(; str[ptr] != delim && 
				  str[ptr] != NEWLINE && 
				  str[ptr] != '\0'; ptr++)
			{
				result *= 10;
				result += str[ptr] - '0';
			}

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
	private:
		static std::list<TData> _filter_by_user_id(Database& database, unsigned int _user_id)
		{
			// Search in database
			auto range = database.map.equal_range(_user_id);

			std::list<TData> lst;
			for(auto it = range.first; it != range.second; ++it)
				lst.push_back(it->second);

			return lst;
		}

	//
	// get()
	//
	private:
		static std::list<Entry> _filter_by_user_id_wrapper(Database& database, unsigned int _user_id)
		{
			// Reset the stream
			database.stream.clear();

			// Start conversion
			std::list<Entry> lst;
			for(const auto& elem : _filter_by_user_id(database, _user_id))
			{
				std::string buffer;
				database.stream.seekg(elem, database.stream.beg);
				std::getline(database.stream, buffer);

				lst.push_back(Entry(buffer));
			}

			return lst;
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

			return std::pair<unsigned int, unsigned long>(clicks, impression);
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
					lst.push_back(std::pair<unsigned int, unsigned int>(elem.get_ad_id(), elem.get_query_id()));
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
		static std::map<unsigned int, std::list<Entry> > impressed(Database& database,
																   unsigned int _user_id_1, unsigned int _user_id_2)
		{
			// Acquire the intersected ads between both users
			std::list<Entry> user_1 = _filter_by_user_id_wrapper(database, _user_id_1);
			std::list<Entry> user_2 = _filter_by_user_id_wrapper(database, _user_id_2);
			std::list<Entry> intersection;
			// Sort the list
			user_1.sort(ad_id_list_comparer);
			user_2.sort(ad_id_list_comparer);
			// Find the intersected ads between user1 and user2
			std::set_intersection(user_1.begin(), user_1.end(),
								  user_2.begin(), user_2.end(),
								  std::back_inserter(intersection),
								  [](Entry const& lhs, Entry const& rhs) { return lhs.get_ad_id() < rhs.get_ad_id(); } );

			// Refine the result for map
			std::map<unsigned int, std::list<Entry> > map;
			for(const auto& elem : intersection)
			{
				// Append the property into list if exists
				if(map.count(elem.get_ad_id()))
					map[elem.get_ad_id()].push_back(elem);
				else
				{
					std::list<Entry> tmp_lst;
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
	};
}

#endif
#ifndef _AD_DATABASE_H
#define _AD_DATABASE_H

#include <iostream>
#include <fstream>
#include <type_traits>

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
			construct_tree(stream, map);
		}

		~Database()
		{
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
					throw "parse_field(): Field out of range.";
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

		void composite_entry(const std::string& str)
		{

		}

	public:
		Entry()
		{

		}

		// Functions for filtering
		bool isGet(unsigned int _user_id,
				   unsigned int _ad_id,
				   unsigned int _query_id,
				   unsigned char _position,
				   unsigned char _depth)
		{
			return (_user_id == user_id) &&
				   (_ad_id == ad_id) &&
				   (_query_id == query_id) &&
				   (_position == position) &&
				   (_depth == depth);
		}

		bool isClicked()
		{
			return click > 0;
		}
	};

	class KDD
	{
		friend Database;

	private:
		// list<template type>, input<filter, using template class>
		std::list<std::pair<unsigned short, unsigned int> > get(unsigned int _user_id)
		{

		}

		std::list<std::pair<unsigned short, unsigned int> > get(unsigned int _user_id,
															    unsigned int _ad_id,
															    unsigned int _query_id,
															    unsigned char _position,
															    unsigned char _depth)
		{

		}

		//double getCTR()

	public:
		// Output the sum of click and impression.
		std::pair<unsigned int, unsigned long> get(unsigned int _user_id,
												   unsigned int _ad_id,
												   unsigned int _query_id,
												   unsigned char _position,
												   unsigned char _depth)
		{

		}

		// Sort by ad_id, than query_id, in ascending mode.
		std::list<std::pair<unsigned int, unsigned int> > clicked(unsigned int _user_id)
		{

		}


	};
}

#endif
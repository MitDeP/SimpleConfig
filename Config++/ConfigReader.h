#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <regex>

/*
Class for managing the reading of a simple configuration file
*/
class ConfigReader
{
public:
	ConfigReader(bool keys_case_insensitive = true, bool exit_on_invalid_key = false, bool exit_on_invalid_entry = true, bool exit_file_not_found = false, bool exit_on_missing_entry = true, 
		bool exit_on_invalid_line = true, bool delay_exit = true);
	~ConfigReader();

	std::unordered_map<std::string, std::string> readFile(std::filesystem::path path);
	std::unordered_map<std::string, std::string> readFile(std::string path);


	bool addKey(std::string key, std::regex validator, bool required = true);
	bool updateKey(std::string key, std::regex validator, bool required = true);
	bool removeKey(std::string key);


private:
	// If true, keys will be case-insensitive. Otherwise case will matter
	bool keys_case_insensitive;
	// If a key is read that is unknown or if a key is inserted twice, fail
	bool exit_on_invalid_key;
	// If an entry fails validation, fail
	bool exit_on_invalid_entry;
	// If the config file is not found, fail
	bool exit_file_not_found;
	// If a key is found but no value is matched, fail
	bool exit_on_missing_entry;
	// If a line fails processing, fail
	bool exit_on_invalid_line;
	// If we are supposed to fail, wait until the whole
	//	file is processed in order to view all errors
	bool delay_exit;
	// True if we have a delayed exit.
	bool due_exit = false;

	std::unordered_map<std::string, std::regex> config_schema;
	std::unordered_map<std::string, bool> entry_required;

	std::unordered_map<std::string, std::string> invalid_entries;
	std::vector<std::string> missing_entries;
	std::vector<std::string> read_errors;


	void selfCheck(std::unordered_map<std::string, std::string> parsed_config);

};


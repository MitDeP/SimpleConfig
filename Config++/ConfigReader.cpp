#include "pch.h"
#include "ConfigReader.h"
#include <fstream>
#include <iostream>
#include <regex>

ConfigReader::ConfigReader(bool keys_case_insensitive, bool exit_on_invalid_key, bool exit_on_invalid_entry, bool exit_file_not_found, bool exit_on_missing_entry, 
	bool exit_on_invalid_line, bool delay_exit){

	this->keys_case_insensitive = keys_case_insensitive;
	this->exit_on_invalid_key = exit_on_invalid_key;
	this->exit_on_invalid_entry = exit_on_invalid_entry;
	this->exit_file_not_found = exit_file_not_found;
	this->exit_on_missing_entry = exit_on_missing_entry;
	this->exit_on_invalid_line = exit_on_invalid_line;
	this->delay_exit = delay_exit;

}

ConfigReader::~ConfigReader() {}

/*
* Function for adding a new key to the reader's schema
* @param key - the key we are adding to the schema
* @param validator - a regular expression for validating the key's assigned value
* @param required - if true, this key is required to be specified (default is true)
* 
* @returns true if the add operation was successful
*/
bool ConfigReader::addKey(std::string key, std::regex validator, bool required) {

	bool added_okay = true;

	if (keys_case_insensitive)
		std::transform(key.begin(), key.end(), key.begin(), ::toupper); 

	if (entry_required.count(key)) {
		std::cerr << "ERROR: Specified key '" << key << "' already in table\n";
		if (exit_on_invalid_key)
			exit(EXIT_FAILURE);
		added_okay = false;
	}
	else {
		entry_required[key] = required;
		config_schema[key] = validator;
	}
	

	return added_okay;

}

/*
* Function for adding a new key to the reader's schema
* @param key - the key we are adding to the schema
* @param validator - a string regex for validating the key's assigned value
* @param required - if true, this key is required to be specified (default is true)
*
* @returns true if the add operation was successful
*/
bool ConfigReader::addKey(std::string key, std::string validator, bool required) {
	return addKey(key, std::regex(validator), required);
}

/*
* Function for removing a key from the configuraiton schema
* @param key - the key being removed
* 
* @returns true if the remove operation succeeded
*/
bool ConfigReader::removeKey(std::string key) {

	bool removed_okay = true;

	if (keys_case_insensitive)
		std::transform(key.begin(), key.end(), key.begin(), ::toupper);

	if (config_schema.count(key) == 0) {
		std::cerr << "ERROR: Could not remove key '" << key << "' - does not exist\n";
		removed_okay = false;
	}
	else {
		entry_required.erase(key);
		config_schema.erase(key);
	}

	return removed_okay;
}

/*
* Function for modifying the schema for a given key
* @param key - the key whose schema is being modified
* @param validator - the modified validator expression
* @param required - if this key is required (default is true)
* @returns true if the operation succeeded
*/
bool ConfigReader::updateKey(std::string key, std::regex validator, bool required) {

	return removeKey(key) && addKey(key, validator, required);
}

/*
* Function for modifying the schema for a given key
* @param key - the key whose schema is being modified
* @param validator - the modified validator expression as a string
* @param required - if this key is required (default is true)
* @returns true if the operation succeeded
*/
bool ConfigReader::updateKey(std::string key, std::string validator, bool required) {
	return updateKey(key, std::regex(validator), required);
}

/*
* Function for getting a map of keys and values from a config file.
* @param path - a string file path to the config file.
* @returns an unordered map of all the keys and values parsed from teh config file.
*/
std::unordered_map<std::string, std::string> ConfigReader::readFile(std::string path) {
	

	if (config_schema.size() == 0)
		std::cerr << "WARNING: No config file schema specified!\n";
	
	std::unordered_map<std::string, std::string> parsed_config = {};

	std::string line;				// Line we are reading
	std::string temp;				// Temporary buffer for manipulation
	std::string key;				// Key we parsed
	std::string value;				// Value we got
	std::ifstream infile(path);		// Filestream of file we are reading
	std::smatch captures;			// Matched groups
	std::string errString;			// Container for error messages
	unsigned int line_number = 0;	// Line number we are on

	// Open file
	if (infile.is_open()) {

		std::regex comments(R"(#[\s\w\W]+)");				// Regex for comments
		//std::regex comments(R"(#[\s\w\W]+)");
		std::regex whitespace(R"([\s]+)");					// Regex for whitespace
		//std::regex whitespace(R"([\s]+)");
		std::regex assignments(R"(([\w]+)=([\w\W]+))");	// Regex for assignment groups
		//std::regex assignments(R"(([\w]+)=([\w\W]+))")


		// Load file line into string variable
		while (getline(infile, line)) {
			line_number++;
			// Reset values for this readthrough
			temp = "";
			key = "";
			value = "";
			errString = "";

			// Replace all comments with empty space
			std::regex_replace(std::back_inserter(temp), line.begin(), line.end(), comments, "");
			line = temp;
			temp = "";

			// Normalize whitespace
			std::regex_replace(std::back_inserter(temp), line.begin(), line.end(), whitespace, "");

			// We found something that matched
			if (std::regex_match(temp, assignments)) {

				// Load the capture groups
				regex_search(temp, captures, assignments);

				// Make sure there are the right number of groups
				if (captures.size() == 3) {

					// Load in key and value pair
					key = captures[1];
					value = captures[2];

					if(keys_case_insensitive)
						std::transform(key.begin(), key.end(), key.begin(), ::toupper);


					// Check if key is known
					if (config_schema.count(key)) {

						// Check if value passes validation
						if (std::regex_match(value, config_schema[key])) {
							
							if (parsed_config.count(key))
								std::cerr << "WARNING: value for key '" << key << "' has multiple entries. Updatring from '" << parsed_config[key] << "' to '" << value << "'\n";

							// Add the value
							parsed_config[key] = value;

						}
						// Validation failed
						else {
							errString = "Value '" + value + "' did not pass validation for '" + key + "' on line " + std::to_string(line_number);
							if (exit_on_invalid_entry && !delay_exit) {
								std::cerr << "ERROR: " << errString << "\n";
								exit(EXIT_FAILURE);
							}
							else if (exit_on_invalid_entry) {
								read_errors.push_back(errString);
								due_exit = true;
							}
							else if(entry_required[key])
								std::cerr << "ERROR: " << errString << "\n";
							else
								std::cerr << "WARNING: " << errString << "\n";
						}

					}
					// Key does not exist
					else {
						errString = "Error on line " + std::to_string(line_number) + ", '" + key + "' - Unknown key";
						if (exit_on_invalid_key && !delay_exit) {
							std::cerr << "ERROR: " << errString << "\n";
							exit(EXIT_FAILURE);
						}
						else if (exit_on_invalid_key) {
							read_errors.push_back(errString);
							due_exit = true;
						}
						else
							std::cerr << "WARNING: " << errString << "\n";
					}

				}
				else if(temp != "") {
					errString = "Syntax error on line " + std::to_string(line_number) + ", '" + temp + "' - line matched pattern but an invalid number of captures were found";
					if (exit_on_invalid_line && !delay_exit) {
						std::cerr << "ERROR: " << errString << "\n";
						exit(EXIT_FAILURE);
					}
					else if (exit_on_invalid_line) {
						read_errors.push_back(errString);
						due_exit = true;
					}
					else
						std::cerr << "WARNING: " << errString << "\n";
				}
			}
			// We found something that did not match out capture group and is not an emptry string
			else if (temp != "") {
				errString = "Syntax error on line " + std::to_string(line_number) + ", '" + temp + "' - did not match any expressions";
				if (exit_on_invalid_line && !delay_exit) {
					std::cerr << "ERROR: " << errString << "\n";
					exit(EXIT_FAILURE);
				}
				else if (exit_on_invalid_line) {
					read_errors.push_back(errString);
					due_exit = true;
				}
				else
					std::cerr << "WARNING: " << errString << "\n";
			}
		}
		infile.close();
	}
	else {
		std::cerr << "ERROR: Could not open " << path << "\n";
		if (exit_file_not_found)
			exit(EXIT_FAILURE);
	}

	selfCheck(parsed_config);

	return parsed_config;

}

/*
* Function for getting a map of keys and values from a config file.
* @param path - a filesystem object specifying the path
* @returns an unordered map of all the keys and values parsed from teh config file.
*/
std::unordered_map<std::string, std::string> ConfigReader::readFile(std::filesystem::path path) {
	return readFile(path.string());
}

/*
* Function for self checking if the config reader's result was valid. If the program is supposed
*	to exit on a delay, it will be executed here after the all of the issues with the config
*	file are displayed to the user.
* @param parsed_config - the dictionary of scraped keys and values from the config file
*/
void ConfigReader::selfCheck(std::unordered_map<std::string, std::string> parsed_config) {

	std::string errString;

	// Make sure all required keys are present
	for (std::unordered_map<std::string, bool>::iterator iter = entry_required.begin(); iter != entry_required.end(); iter++) {
		errString = "";
		// Missing required entry
		if (parsed_config.count(iter->first) == 0 && iter->second) {
			errString = "Missing value for '" + iter->first + "' is required";
			if (exit_on_missing_entry && !delay_exit) {
				std::cerr << "ERROR: " << errString << "\n";
				exit(EXIT_FAILURE);
			}
			else if (exit_on_missing_entry) {
				missing_entries.push_back(iter->first);
				due_exit = true;
			}
			else
				std::cerr << "WARNING: " << errString << "\n";
		}
	}

	if (missing_entries.size()) {
		std::clog << "\nThe following required entries are missing:\n";
		for (std::string miss : missing_entries)
			std::clog << "\t" << miss << "\n";

		std::clog << "\n\n";
	}

	if (read_errors.size()) {
		std::clog << "\nWhile reading the config file, the following errors occured:\n";
		for (std::string err : read_errors)
			std::clog << "\t" << err << "\n";

		std::clog << "\n\n";
	}

	if (due_exit) {
		std::clog << "\nDue to errors reading the config file, the program will now terminate\n";
		exit(EXIT_FAILURE);
	}

}
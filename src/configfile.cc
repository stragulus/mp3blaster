#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "configfile.h"

#ifndef OLD_GCC
using namespace std;
#endif

ConfigFile::ConfigFile() {
}

ConfigFile::~ConfigFile() {
}

const string
ConfigFile::getError() {
	return error_;
}

void
ConfigFile::load(const char *filename) {
	ifstream file(filename);
	string line;
	unsigned int linecount;
	bool parseError = false;
	
	if (!file) {
		throw ConfigFileException(string("Could not open configuration file"));
	}

	linecount = 0;
	while ( !parseError && (getline(file, line)) ) {
		parseError = !parseLine(line);		
		if (parseError) {
			ostringstream s;
			
			s << "Error on line " << linecount + 1 << ": " << getError();
			setError(s.str());
		}
		linecount++;
	}

	if (parseError)
		return false; //error has been set

	if (!file.eof()) {
		setError("A read error occurred while trying to read file");
		return false;
	}

	return true;
}

bool
ConfigFile::setError(const string str) {
	error_ = str;
	return false; //convenient, you can do return setError() in a fun.
}

bool
ConfigFile::parseLine(const string l) {
	char
		keyword[l.length() + 1],
		tmpvals[l.length() + 1],
		prev = '\0';
	unsigned int 
		i;
	string
		current_word,
		line = l;
	values_t
		values;

	//TODO: function that crops whitespace from string objects.
	if (!line.length() || line[0] == '#')
		return true;

	// ignore empty lines, also those with only whitespace in them
	cropWhitespace(line);

	if (!line.length())
		return true;

	if (sscanf(line.c_str(), " %[^ =\n] = %[^\n]\n", keyword, tmpvals) != 2) {
		setError("Failed to parse this line (there's no '=')");
		return false;
	}

	//tmpvals = keyword[s];
	i = 0; //curval_pos = 0; val_index = 0;
	current_word = "";
	while (i < strlen(tmpvals)) {
		char curr = tmpvals[i];
		if (prev == '\\') {
			//previous read char was a backslash. That implies that this 
			//character is treated a 'special' character. Currently only
			//comma and backslash itself.
			switch(curr) {
				case '\\':
				case ',':
					current_word += curr;
					break;
			}
			prev = '\0'; // prevent special char interpr. in next loop ;)
		} else if (curr == '\\') {
			prev = '\\';
		} else if (curr == ',') {
			// keyword separator
			cropWhitespace(current_word);
			values.push_back(current_word);
			current_word = "";
			prev = ',';
		} else {
			current_word += curr;
			prev = curr;
		}

		i++;
	}

	cropWhitespace(current_word);
	values.push_back(current_word);
	lines_[keyword] = values;

	return true;
}

void
ConfigFile::cropWhitespace(string &s) {
	while (s.length() && (s[0] == ' ' || s[0] == '\t'))
			s.erase(s.begin());
	while (s.length() && (s[s.length() - 1] == ' ' || s[s.length() - 1] == '\t'))
			s.erase(s.end());
}

void
ConfigFile::dumpOutput(ostringstream &o) {
	cf_line_t::iterator it = lines_.begin();
	unsigned int count = 0;
	values_t values;

	for (it =lines_.begin(); it != lines_.end(); it++) {
		values_t values = it->second;

		o << "Entry " << count + 1 << ": " << it->first << " = ";

		for (values_t::iterator v_it = values.begin(); v_it != values.end();
				v_it++) {
			o << *v_it << ", ";
		}

		o << endl;

		count++;
	}
}

vector< string >
ConfigFile::getEntry(const string keyword) {
	// TODO: Check if this thing actually exists in the mapping
	return lines_[keyword];
}

bool
ConfigFile::getKeyBinding(const string keyword, vector<string> &k) {
	values_t v = lines_[keyword];
	string result;
	bool success = true;

	if (!v.size())
		return setError("Entry not found");

	k.clear();
	k.resize(v.size());

	for (values_t::iterator it = v.begin(); it != v.end(); it++) {
		if (!isValidKeyBinding(*it)) {
			success = false;
			break;
		}
		k.push_back(*it);
	}

	return success;
}

bool
ConfigFile::getKeyBinding(const string keyword, string &k) {
	values_t v = lines_[keyword];
	string result;

	if (!v.size())
		return setError("Entry not found");
	if (v.size() > 1)
		return setError("Entry is a list, while that is not allowed");

	result = v[0];

	if (isValidKeyBinding(result)) {
		k = result;
		return true;
	}

	return false;
}

bool
ConfigFile::getBoolean(const string keyword, bool &b) {
	values_t v = lines_[keyword];

	if (!v.size())
		return setError("Entry not found");
	if (v.size() > 1)
		return setError("Entry is a list, while that is not allowed");

	if (!isBoolean(v[0], b))
		return setError("Value is not a boolean");

	return true;
}

bool
ConfigFile::getBoolean(const string keyword, vector<bool> &b) {
	values_t
		v = lines_[keyword];

	if (!v.size())
		return setError("Entry not found");

	b.clear();
	b.resize(v.size());

	for (values_t::iterator it = v.begin(); it != v.end(); it++) {
		bool
			tmpbool;

		if (!isBoolean(*it, tmpbool))
			return setError("Value is not a boolean");

		b.push_back(tmpbool);
	}
	return true;
}

bool
ConfigFile::getColour(const string keyword, string &c) {
	values_t v = lines_[keyword];

	if (!v.size())
		return setError("Entry not found");
	if (v.size() > 1)
		return setError("Entry is a list, while that is not allowed");

	if (!isColour(v[0]))
		return setError("Value is not a boolean");

	c = v[0];
	return true;
}

bool
ConfigFile::getColour(const string keyword, vector<string> &c) {
	values_t v = lines_[keyword];

	if (!v.size())
		return setError("Entry not found");

	c.clear();
	c.resize(v.size());

	for (values_t::iterator it = v.begin(); it != v.end(); it++) {
		if (!isColour(*it))
			return setError("Value is not a boolean");

		c.push_back(*it);
	}
	return true;
}


bool
ConfigFile::getNumber(const string keyword, int &n) {
	values_t v = lines_[keyword];

	if (!v.size())
		return setError("Entry not found");
	if (v.size() > 1)
		return setError("Entry is a list, while that is not allowed");

	n = atoi(v[0].c_str());	
	return true;
}

bool
ConfigFile::getNumber(const string keyword, vector<int> &n) {
	values_t v = lines_[keyword];

	if (!v.size())
		return setError("Entry not found");

	n.clear();
	n.resize(v.size());

	for (values_t::iterator it = v.begin(); it != v.end(); it++) {
		int number = atoi((*it).c_str());
		n.push_back(number);
	}
	return true;
}

bool
ConfigFile::hasEntry(const string keyword) {
	return 0 != lines_[keyword].size();
}

/* returns a NULL-terminated array of strings (char-pointers), or NULL when
 * the entry does not exist. */
char **
ConfigFile::getEntry(const char *keyword) {
	values_t v;
	char **result = NULL;
	unsigned int i;

	// TODO: How do I check if a mapping actually has an element 'keyword'?
	v = lines_[keyword];

	if (!v.size())
			return NULL;

	result = (char **)malloc((v.size() + 1) * sizeof(char *));

	i = 0;
	for (values_t::iterator it = v.begin(); it != v.end(); it++)
		result[i++] = strdup((*it).c_str());

	result[i] = NULL;

	return result;
}

bool
ConfigFile::isBoolean(const string str, bool &b) {
	const char *val = str.c_str();

	if (!val)
		return false;

	if (!strcasecmp(val, "yes") || !strcasecmp(val, "true") ||
			atoi(val) == 1) {
		b = true;
	} else if (!strcasecmp(val, "no") || !strcasecmp(val, "false") ||
			atoi(val) == 0) {
		b = false;
	} else {
		return false;
	}
}

bool
ConfigFile::isValidKeyBinding(const string &result) {
	const char *keyBindings[] = { 
		"spc", "ent", "ret", "ins", "del", "hom", "end", "pup", "pdn", "up",
		"dwn", "lft", "rig", "bsp",
		NULL };
	unsigned int i = 0;
	int dum;

	//canonical keybinding ?
	while (keyBindings[i]) {
		if (!strcasecmp(result.c_str(), keyBindings[i]))
			return true;
		i++;
	}

	//single character? 
	if (result.size() == 1)
		return true;

	//function key?
	if (sscanf(result.c_str(), "f%d", &dum) && dum >= 0 && dum < 64)
		return true;

	//keypad?
	if (sscanf(result.c_str(), "kp%d", &dum) && dum >= 0 && dum < 10)
		return true;

	//scan code?
	if (sscanf(result.c_str(), "s%x", &dum) && dum >= 0 && dum < 256)
		return true;

	//nothing appropriate matched.
	return false;
}

bool
ConfigFile::isColour(const string &result) {
	const char *clrs[] = {
		"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
		NULL };
	unsigned int i = 0;

	while (clrs[i]) {
		if (!strcasecmp(result.c_str(), clrs[i]))
			return true;
		i++;
	}

	return false;
}

ConfigFileException::ConfigFileException(string message) {
	message_ = message;
}

const char*
ConfigFileException::what() {
	return message.c_string();
}


#ifndef _CONFIGFILE_CLASS_
#define _CONFIGFILE_CLASS_

#include <exception>
#include <string>
#include <vector>
#include <map>
#include <sstream>

class ConfigFile {
	public:
		typedef std::vector< std::string > values_t;
		typedef std::map< std::string, values_t, std::less< std::string > > cf_line_t;

		ConfigFile();
		~ConfigFile();

		void load(const char *filename);
		const std::string getError();
		void dumpOutput(std::ostringstream &o);
		values_t getEntry(const std::string keyword);
		char **getEntry(const char *keyword);
		bool hasEntry(const std::string keyword);
		bool getNumber(const std::string keyword, int &n);
		bool getNumber(const std::string keyword, std::vector<int> &n);
		bool getBoolean(const std::string keyword, bool &b);
		bool getBoolean(const std::string keyword, std::vector<bool> &b);
		bool getKeyBinding(const std::string keyword, std::string &k);
		bool getKeyBinding(const std::string keyword, std::vector<std::string> &k);
		bool getColour(const std::string keyword, std::string &c);
		bool getColour(const std::string keyword, std::vector<std::string> &c);

	protected:
		bool setError(const std::string str);
		void cropWhitespace(std::string &s);

	private:
		bool parseLine(const std::string l);
		bool isBoolean(const std::string str, bool &b);
		bool isValidKeyBinding(const std::string &result);
		bool isColour(const std::string &result);

		std::string error_;
		cf_line_t lines_;
};

class ConfigFileException: public std::exception {
	public:
		ConfigFileException(std::string message);

		virtual const char* what() const throw();

	private:
		std::string message_;
};

#endif /* _CONFIGFILE_CLASS_ */

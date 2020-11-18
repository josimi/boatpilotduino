// FileParser
// Jonathon Simister

#ifndef FileParser_h
#define FileParser_h

#include "SD.h"

class FileParser {
	public:
		FileParser(const char* filename);
		~FileParser();
		
		bool expectString(const char* s);
		
		bool readFloat(float* out);
		bool readChar(char* out);
		bool readKeyword(const char** keywords, int n, const char** out);
	private:
		void readWhitespace();
	
		File file;
	
		int bi;
		char buf[512];
		int bl;
};

#endif

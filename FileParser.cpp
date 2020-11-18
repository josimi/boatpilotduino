// FileParser
// Jonathon Simister

#include "FileParser.h"

FileParser::FileParser(const char* filename) {	
	this->file = SD.open(filename);
	
	if(this->file.available() > 0) {
		this->bi = 0;
		this->bl = min(this->file.available(), 512);
		this->file.read(this->buf, this->bl);
	}
}

FileParser::~FileParser() {
	this->file.close();
}

bool FileParser::readChar(char* out) {
	if(this->bi < this->bl) {
		*out = this->buf[this->bi++];
		return true;
	}
	
	if(this->file.available() > 0) {
		this->bi = 0;
		this->bl = min(this->file.available(), 512);
		this->file.read(this->buf, this->bl);
		*out = this->buf[this->bi++];
		return true;
	}
	
	return false;
}

void FileParser::readWhitespace() {
	char ch;
	
	while(readChar(&ch)) {
		// ignore comments
		if(ch == '#') {
			while(readChar(&ch) && ch != '\n') { }
			continue;
		}
		
		if(ch != ' ' && ch != '\n' && ch != '\t' && ch != '\r') {
			this->bi--;
			break;
		}
	}
}

bool FileParser::expectString(const char* s) {
	int i = 0;
	char ch;
	
	readWhitespace();
	
	while(s[i] != 0) {
		if(!readChar(&ch)) { return false; }
		if(ch != s[i]) { return false; }
    
    i++;
	}

	return true;
}

bool FileParser::readKeyword(const char** keywords, int n, const char** out) {
	int left = 0;
	int right = n;
	const char* match = NULL;

	readWhitespace();
	
	char ch;
	
	int y = 0;
	while(right - left > 0 && readChar(&ch)) {
		for(int x = left;x < right;x++) {
			if(keywords[x][y] == 0) { match = keywords[x]; }
			if(keywords[x][y] < ch) { left = x + 1; }
			if(keywords[x][y] > ch) { right = x; }
		}
		
		y++;
	}

	const char* s;
	
	if(match != NULL) {
		*out = match;

		this->bi--;
		s = this->buf + this->bi;

		return true;
	}

	this->bi -= y;

	s = this->buf + this->bi;

	return false;
}

bool FileParser::readFloat(float* out) {
	bool neg = false;
	bool digits = false;
	bool sign = false;
	bool decimal = false;
	float integer = 0;
	float fraction = 0;
	float divisor = 1;
	
	readWhitespace();
	
	char ch;
	while(readChar(&ch)) {
		if(ch == '-') {
			if(sign || digits) { this->bi--; break; }
			sign = true;
			neg = true;
		} else if(ch >= '0' && ch <= '9') {
			digits = true;
			if(decimal) {
				fraction *= 10;
				fraction += ch-'0';
				divisor *= 10;
			} else {
				integer *= 10;
				integer += ch-'0';
			}
		} else if(ch == '.') {
			if(decimal) { this->bi--; break; }
			decimal = true;
		} else {
			this->bi--; break;
		}
	}
	
	if(digits) {
		float n = integer + (fraction / divisor);
    if(neg) { n = copysign(n, -1); }

    *out = n;
   
		return true;
	}
	return false;
}

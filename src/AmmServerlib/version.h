#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "07";
	static const char MONTH[] = "07";
	static const char YEAR[] = "2014";
	static const char UBUNTU_VERSION_STYLE[] =  "14.07";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 27;
	static const long BUILD  = 101;
	static const long REVISION  = 388;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 86;
	#define RC_FILEVERSION 0,27,101,388
	#define RC_FILEVERSION_STRING "0, 27, 101, 388\0"
	static const char FULLVERSION_STRING [] = "0.27.101.388";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 66;
	

#endif //VERSION_H

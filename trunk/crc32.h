#ifndef _CRC32STATIC_H_
#define _CRC32STATIC_H_

// Read 4K of data at a time (used in the C++ streams, Win32 I/O, and assembly functions)
#define MAX_BUFFER_SIZE 4096

// Map a "view" size of 10MB (used in the filemap function)
#define MAX_VIEW_SIZE   10485760

class CCrc32Static
{
public:
    CCrc32Static();
    virtual ~CCrc32Static();

    static unsigned int StringCrc32(const char* szString, unsigned int &dwCrc32);
    static unsigned int FileCrc32Streams(const char* szFilename, unsigned int &dwCrc32);

protected:
    static inline void CalcCrc32(const unsigned char byte, unsigned int &dwCrc32);

    static unsigned int s_arrdwCrc32Table[256];
};

#endif

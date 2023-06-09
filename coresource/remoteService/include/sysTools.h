#ifndef SYSTOOLS_H
#define SYSTOOLS_H
#include <string>
#include <string.h>
#include <sstream>
#include <iconv.h>
 #include <fcntl.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <dirent.h>
#include <sys/stat.h>
#include "datatype.h"

#define BUFFER_SIZE 1024
#define SUCCESS 1
#define FAILURE 0

using namespace std;
class sysTools
{
    public:
        sysTools();
        virtual ~sysTools();

        static string   sstrToHex(string str);
        static int      str_to_hex(char *string, unsigned char *cbuf, int len);

        static int      copyFile(const char *sourceFileNameWithPath,const char *targetFileNameWithPath);
        static int      copyDir(const char *sourceFileDir,const char *targetFileDir);
        static void     Getfilepath(const char *path, const char *filename,  char *filepath);
        static bool     DeleteFile(const char* path);

    protected:
    private:
};

#endif // SYSTOOLS_H

#include "sysTools.h"

sysTools::sysTools()
{
    //ctor
}

sysTools::~sysTools()
{
    //dtor
}

/**
* #purpose	: 字符串转十六进制字符串
* #note	: 可用于汉字字符串
* #param str		: 要转换成十六进制的字符串
* #param separator	: 十六进制字符串间的分隔符
* #return	: 接收转换后的字符串
*/
string sysTools::sstrToHex(string str)
{
    string separator = "";
	const std::string hex = "0123456789ABCDEF";
	std::stringstream ss;

	for (std::string::size_type i = 0; i < str.size(); ++i)
		ss << hex[(unsigned char)str[i] >> 4] << hex[(unsigned char)str[i] & 0xf] << separator;

	return ss.str();
}

/**
*#purpose	: 十六进制字符串转十六进制数组
*#param string		:十六进制字符串
*#param cbuf        :十六进制数组
*#param len         :长度
*#return            :成功返回0  失败返回-1
*/
int sysTools::str_to_hex(char *string, unsigned char *cbuf, int len)
{
    ST_BYTE high, low;
    int idx, ii=0;
    for (idx=0; idx<len; idx+=2)
    {
        high = string[idx];
        low = string[idx+1];

        if(high>='0' && high<='9')
            high = high-'0';
        else if(high>='A' && high<='F')
            high = high - 'A' + 10;
        else if(high>='a' && high<='f')
            high = high - 'a' + 10;
        else
            return -1;

        if(low>='0' && low<='9')
            low = low-'0';
        else if(low>='A' && low<='F')
            low = low - 'A' + 10;
        else if(low>='a' && low<='f')
            low = low - 'a' + 10;
        else
            return -1;

        cbuf[ii++] = high<<4 | low;
    }
    return 0;
}

int sysTools::copyFile(const char *sourceFileNameWithPath,
        const char *targetFileNameWithPath)
{
	FILE *fpR, *fpW;
	char buffer[BUFFER_SIZE];
	int lenR, lenW;
	if ((fpR = fopen(sourceFileNameWithPath, "r")) == NULL)
	{
		printf("The file '%s' can not be opened! \n", sourceFileNameWithPath);
		return FAILURE;
	}
	if ((fpW = fopen(targetFileNameWithPath, "w")) == NULL)
	{
		printf("The file '%s' can not be opened! \n", targetFileNameWithPath);
		fclose(fpR);
		return FAILURE;
	}

	memset(buffer, 0, BUFFER_SIZE);
	while ((lenR = fread(buffer, 1, BUFFER_SIZE, fpR)) > 0)
	{
		if ((lenW = fwrite(buffer, 1, lenR, fpW)) != lenR)
		{
			printf("Write to file '%s' failed!\n", targetFileNameWithPath);
			fclose(fpR);
			fclose(fpW);
			return FAILURE;
		}
		memset(buffer, 0, BUFFER_SIZE);
	}

	fclose(fpR);
	fclose(fpW);
	printf("copy file '%s' successful!\r\n",targetFileNameWithPath);
	return SUCCESS;
}

int sysTools::copyDir(const char *sourceFileDir,const char *targetFileDir)
{
    DIR *dir;
    struct dirent *dirinfo;
    if ((dir = opendir(sourceFileDir)) == NULL){
        printf("dir is empty!");
        return FAILURE;
    }
    while ((dirinfo = readdir(dir)) != NULL)
    {
        if (strcmp(dirinfo->d_name, ".") == 0 || strcmp(dirinfo->d_name, "..") == 0)//判断是否是特殊目录
            continue;

        string sPath(sourceFileDir);
        string tPath(targetFileDir);

        sPath.append(dirinfo->d_name);
        tPath.append(dirinfo->d_name);
        int res = copyFile(sPath.c_str(),tPath.c_str());
        if(!res)
            return res;
    }
    return SUCCESS;
}

void sysTools::Getfilepath(const char *path, const char *filename,  char *filepath)
{
    strcpy(filepath, path);
    if(filepath[strlen(path) - 1] != '/')
        strcat(filepath, "/");
    strcat(filepath, filename);
	printf("path is = %s\n",filepath);
}
/**
*#purpose	:删除文件夹内数据
*/
bool sysTools::DeleteFile(const char* path)
{
    DIR *dir;
    struct dirent *dirinfo;
    struct stat statbuf;
    char filepath[256] = {0};
    lstat(path, &statbuf);

    if (S_ISREG(statbuf.st_mode))//判断是否是常规文件
    {
        remove(path);
    }
    else if (S_ISDIR(statbuf.st_mode))//判断是否是目录
    {
        if ((dir = opendir(path)) == NULL)
            return 1;
        while ((dirinfo = readdir(dir)) != NULL)
        {
            Getfilepath(path, dirinfo->d_name, filepath);
            if (strcmp(dirinfo->d_name, ".") == 0 || strcmp(dirinfo->d_name, "..") == 0)//判断是否是特殊目录
            continue;
            DeleteFile(filepath);
            rmdir(filepath);
        }
        closedir(dir);
    }
    return 0;
}

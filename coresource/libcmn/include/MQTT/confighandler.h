//ConfigHandler.h  
#pragma once  
  
#include <string>  
#include <map>  
#include <iostream>  
#include <fstream>  
#include <sstream>  
  
  
/* 
* \brief Generic configuration Class 
* 
*/  
class ConfigHandler {  
    // Data  
private:  
    std::string m_Delimiter;  //!< separator between key and value  
    std::string m_Comment;    //!< separator between value and comments  
    std::map<std::string,std::string> m_Contents;  //!< extracted keys and values  
  
    typedef std::map<std::string,std::string>::iterator mapi;  
    typedef std::map<std::string,std::string>::const_iterator mapci;  
    // Methods  
public:  
  
    ConfigHandler( std::string filename, std::string delimiter = "=",std::string comment = "#" );  
    ConfigHandler();  
    template<class T> T Read( const std::string& in_key, const T& in_value ) const;  
    template<class T> bool ReadInto( T& out_var, const std::string& in_key ) const;  
    template<class T>  
    bool ReadInto( T& out_var, const std::string& in_key, const T& in_value ) const;  
    void ReadFile(std::string filename,std::string delimiter = "=",std::string comment = "#" );  
  
    // Check whether key exists in configuration  
    bool KeyExists( const std::string& in_key ) const;  
  
    // Modify keys and values  
    template<class T> void Add( const std::string& in_key, const T& in_value );  
    void Remove( const std::string& in_key );  
  
    // Check or change configuration syntax  
    std::string GetDelimiter() const { return m_Delimiter; }  
    std::string GetComment() const { return m_Comment; }  
    std::string SetDelimiter( const std::string& in_s )  
    { std::string old = m_Delimiter;  m_Delimiter = in_s;  return old; }    
    std::string SetComment( const std::string& in_s )  
    { std::string old = m_Comment;  m_Comment =  in_s;  return old; }  
  
    // Write or read configuration  
    friend std::ostream& operator<<( std::ostream& os, const ConfigHandler& cf );  
    friend std::istream& operator>>( std::istream& is, ConfigHandler& cf );  
  
private:  
    template<class T> static std::string T_as_string( const T& t );  
    template<class T> static T string_as_T( const std::string& s );  
    static void Trim( std::string& inout_s );  
};  
  
  
/* static */  
template<class T>  
inline std::string ConfigHandler::T_as_string( const T& t )  
{  
    // Convert from a T to a string  
    // Type T must support << operator  
    std::ostringstream ost;  
    ost << t;  
    return ost.str();  
}  
  
  
/* static */  
template<class T>  
inline T ConfigHandler::string_as_T( const std::string& s )  
{  
    // Convert from a string to a T  
    // Type T must support >> operator  
    T t;  
    std::istringstream ist(s);  
    ist >> t;  
    return t;  
}  
  
  
/* static */  
template<>  
inline std::string ConfigHandler::string_as_T<std::string>( const std::string& s )  
{  
    // Convert from a string to a string  
    // In other words, do nothing  
    return s;  
}  
  
  
/* static */  
template<>  
inline bool ConfigHandler::string_as_T<bool>( const std::string& s )  
{  
    // Convert from a string to a bool  
    // Interpret "false", "F", "no", "n", "0" as false  
    // Interpret "true", "T", "yes", "y", "1", "-1", or anything else as true  
    bool b = true;  
    std::string sup = s;  
    for( std::string::iterator p = sup.begin(); p != sup.end(); ++p )  
        *p = toupper(*p);  // make string all caps  
    if( sup==std::string("FALSE") || sup==std::string("F") ||  
        sup==std::string("NO") || sup==std::string("N") ||  
        sup==std::string("0") || sup==std::string("NONE") )  
        b = false;  
    return b;  
}  
  
template<class T>  
inline T ConfigHandler::Read( const std::string& key, const T& value ) const  
{  
    // Return the value corresponding to key or given default value  
    // if key is not found  
    mapci p = m_Contents.find(key);  
    if( p == m_Contents.end() ) return value;  
    return string_as_T<T>( p->second );  
}  
  
  
template<class T>  
inline bool ConfigHandler::ReadInto( T& var, const std::string& key ) const  
{  
    // Get the value corresponding to key and store in var  
    // Return true if key is found  
    // Otherwise leave var untouched  
    mapci p = m_Contents.find(key);  
    bool found = ( p != m_Contents.end() );  
    if ( found ) var = string_as_T<T>( p->second );  
    return found;  
}  
  
  
template<class T>  
inline bool ConfigHandler::ReadInto( T& var, const std::string& key, const T& value ) const  
{  
    // Get the value corresponding to key and store in var  
    // Return true if key is found  
    // Otherwise set var to given default  
    mapci p = m_Contents.find(key);  
    bool found = ( p != m_Contents.end() );  
    if ( found )  
        var = string_as_T<T>( p->second );  
    else  
        var = value;  
    return found;  
}  
  
  
template<class T>  
inline void ConfigHandler::Add( const std::string& in_key, const T& value )  
{  
    // Add a key with given value  
    std::string v = T_as_string( value );  
    std::string key=in_key;  
    Trim(key);  
    Trim(v);  
    m_Contents[key] = v;  
    return;  
}  
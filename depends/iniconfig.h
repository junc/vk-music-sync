#ifndef INIPARSER_H
#define INIPARSER_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>

#include "variant.h"

class IniConfig
{
    std::string _file;
    std::map<std::string, std::map<std::string, Variant> > vars;
    
    static bool checkSlashes(std::string str);
    bool created;
    
public:
    struct stat sb;
    
    IniConfig();
    IniConfig(const char* file);
    
    void setConfig(const char* file);
    void parse();
    void save();
    void reset();
    bool isCreated();
    
    std::map<std::string, Variant> operator[](std::string index);
    
    Variant get(std::string key);
    Variant get(std::string section, std::string key);
    std::map<std::string, std::map<std::string, Variant> > getConfig();
    
    template <class T>
    Variant get(std::string section, std::string key, T _default)
    {
        if (!vars[section].count(key)) {
            vars[section][key] = Variant(_default);
        }
        
        return vars[section][key];
    }
    
    template <class T>
    void set(std::string section, std::string key, T _default)
    {
        vars[section][key] = Variant(_default);
    }
};

#endif // INIPARSER_H

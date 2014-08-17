#include "iniconfig.h"

bool IniConfig::checkSlashes(std::string str)
{
    str = Variant::replace(str, "\\\\", "");
    return (str.length() > 1 && str[str.length()-1] == '"' && str[str.length()-2] == '\\');
}

IniConfig::IniConfig()
{
    created = false;
}

IniConfig::IniConfig(const char *file)
{
    setConfig(file);
}

void IniConfig::setConfig(const char *file)
{
    _file = file;
    created = false;
    
    if (stat(file, &sb) != 0) {
        std::ofstream outfile(file);
        outfile << "";
        outfile.close();
        created = true;
    }
}

void IniConfig::parse()
{
    std::string temp;
    std::string index = "Global"; // Index in ini-file. Example: [Global]
    std::string key;
    std::string value;
    bool newLine = false; // New line with " ... "
    bool newLineBracket = false; // New line with ( ... )
    size_t found;
    
    std::ifstream file;
    file.open(_file);
    
    while (std::getline(file, temp)) {
        // New line inside "..." or (...)
        if (newLine || newLineBracket) {
            value += " ";
            value += Variant::trim(temp);
            
            if (value[value.length() - 1] == '"') {
                newLine = checkSlashes(value);
            } else if (value[value.length() - 1] == ')') {
                newLineBracket = false;
            }
        } else {
            found = temp.find("=");
            if (found != std::string::npos) {
                key   = Variant::trim(temp.substr(0, found));
                value = Variant::trim(temp.substr(found+1));
                
                if (index.length()) {
                    //key = index + "/" + key;
                }
                
                // String inside "..." or (...)
                if ((!newLineBracket && value.length() && value[0] == '"' && value[value.length() - 1] != '"') || checkSlashes(value)) {
                    newLine = true;
                } else if (value.length() && value[0] == '(' && value[value.length() - 1] != ')') {
                    newLineBracket = true;
                }
            } else {
                temp = Variant::trim(temp);
                
                if (temp.length() && temp[0] == '[' && temp[temp.length() - 1] == ']') {
                    index = temp.substr(1, temp.length() - 2);
                }
            }
        }
        
        // Save to config var.
        if (!newLine && !newLineBracket && key.length()) {
            if (value.length()) {
                // Remove \\ and "
                if (value[0] == '"' && value[value.length()-1] == '"') {
                    value.erase(0, 1);
                    value.erase(value.length()-1);
                    
                    value = Variant::replace(value, "\\\\", "\\");
                    value = Variant::replace(value, "\\\"", "\"");
                }
                
                vars[index][key.c_str()] = value.c_str();
            } else {
                vars[index][key.c_str()] = "";
            }
            
            key.clear();
            value.clear();
        }
    }
    
    file.close();
}

void IniConfig::save()
{
    std::string index = "Global";
    std::string lastIndex;
    std::string key;
    std::string value;
    
    std::ofstream file;
    file.open(_file);
    
    for (const auto &section : vars) {
        file << "\n[" << section.first << "]\n";
        
        for (const auto &cfg : vars[section.first]) {
            key = cfg.first;
            
            // Global key. Example: Global/Variable
            index = section.first;
            
            value = vars[index][cfg.first].toString();
            
            if (value.length() && value[0] != '(') {
                value = Variant::replace(value, "\\", "\\\\", 0, false);
                value = Variant::replace(value, "\"", "\\\"", 0, false);
                
                if (value.find_first_of(" \\\";'") != std::string::npos) {
                    value = '"' + value + '"';
                }
            }
            
            
            file << key << " = " << value << "\n";
            lastIndex = index;
        }
    }
    
    file.close();
}

void IniConfig::reset()
{
    if (!_file.empty()) {
        std::ofstream outfile(_file);
        outfile << "";
        outfile.close();
    }
}

bool IniConfig::isCreated()
{
    return created;
}

std::map<std::string, Variant> IniConfig::operator[](std::string index)
{
    return vars[index];
}

Variant IniConfig::get(std::string key)
{
    size_t found = key.find("/");
    if (found != std::string::npos) {
        std::string globalKey = key.substr(0, found);
        key.erase(0, found+1);
        return vars[globalKey][key];
    }
    
    if (vars[""].find(key) != vars[""].end()) {
        return vars[""][key];
    }
    
    if (vars["Global"].find(key) != vars["Global"].end()) {
        return vars["Global"][key];
    }
    
    return Variant("");
}

Variant IniConfig::get(std::string section, std::string key)
{
    return vars[section][key];
}

std::map<std::string, std::map<std::string, Variant> > IniConfig::getConfig()
{
    return vars;
}

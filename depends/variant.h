#ifndef VARIANT_H
#define VARIANT_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>

/*
 * Variant can contains some variables simply types:
 * int, float, double, char, etc, std::string, std::vector
*/
class Variant
{
    std::string _data;
    
    template <class T>
    void asVector(std::vector<T> data);
    
    template <class T>
    void as(T data)
    {
        std::stringstream ss; ss << data;
        _data = ss.str();
    }
    
public:
    Variant();
    Variant(const Variant &data);
    
    template <class T>
    Variant(T data)
    {
        as<T>(data);
    }
    
    template <class T>
    Variant(std::vector<T> data)
    {
        asVector<T>(data);
    }
    
    template <class T>
    T convert()
    {
        std::stringstream ss(_data.c_str());
        T temp; ss >> temp;
        return temp;
    }
    
    void setData(char* data);
    bool isEmpty();
    
    bool toBool();
    char toChar();
    short toShort();
    size_t toSize_t();
    int toInt();
    float toFloat();
    double toDouble();
    long toLong();
    long long toLongLong();
    std::string toString();
    const char* toCharPointer();
    
    static std::string trim(std::string str);
    static std::string replace(std::string str, std::string from, std::string to, size_t n = 0, bool replaceAllMatches = true);
    
    template <class C, class T>
    C to() const
    {
        T temp;
        C out;
        std::string str;
        std::stringstream ss;
        
        bool end       = true;
        size_t slashes = 0;
        
        if (_data == "()") {
            return out;
        }
        
        str = _data;
        
        // Erase '(' and ')'
        str.erase(0, 1);
        str.erase(str.end()-1, str.end());
        
        for (size_t i = 0; i < str.length(); i++) {
            if (end) {
                if (str[i] == '"') {
                    end = false;
                }
                continue;
            }
            
            // Counting sleshes
            if (str[i] == '\\') {
                slashes++;
                if (slashes > 1 && slashes % 2 == 0) {
                    ss << str[i];
                }
                continue;
            }
            
            if (str[i] == '"') {
                if (slashes % 2 == 1) {
                    ss << str[i];
                    slashes = 0;
                    continue;
                }
                
                // For string
                if (ss.str().find(" ") != std::string::npos) {
                    T stemp;
                    T stempFull;
                    while (ss >> stemp) {
                        stempFull += stemp + ' ';
                    }
                    temp = stempFull;
                } else {
                    ss >> temp;
                }
                
                out.push_back(temp);
                
                end = true;
                ss.str("");
                ss.clear();
            } else {
                ss << str[i];
            }
        }
        
        return out;
    }
    
    // Convert to vector.
    template <class T>
    std::vector<T> toVector() const
    {
        return to<std::vector<T>, T>();
    }
};

#endif // VARIANT_H

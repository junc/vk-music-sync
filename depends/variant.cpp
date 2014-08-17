#include "variant.h"

Variant::Variant(const Variant &data)
{
    _data = data._data;
}

bool Variant::isEmpty()
{
    return _data.empty();
}

char Variant::toChar()
{
    return convert<char>();
}

short Variant::toShort()
{
    return convert<short>();
}

size_t Variant::toSize_t()
{
    return convert<size_t>();
}

int Variant::toInt()
{
    return convert<int>();
}

float Variant::toFloat()
{
    return convert<float>();
}

double Variant::toDouble()
{
    return convert<double>();
}

long Variant::toLong()
{
    return convert<long>();
}

long long Variant::toLongLong()
{
    return convert<long long>();
}

std::string Variant::toString()
{
    return _data;
}

const char *Variant::toCharPointer()
{
    return _data.c_str();
}

std::string Variant::trim(std::string str)
{
    std::string whitespaces(" \t\f\v\n\r");
    size_t found = str.find_last_not_of(whitespaces);
    
    if (found != std::string::npos) {
        str.erase(found+1);
    } else {
        str.clear();
    }
    
    found = str.find_first_not_of(whitespaces);
    if (found != std::string::npos) {
        str.erase(0, found);
    }
    
    return str;
}

template <class T>
void Variant::asVector(std::vector<T> data)
{
    std::stringstream ss;
    std::stringstream temp;
    std::string str;
    
    if (data.size() < 2) {
        if (data.size() == 1) {
            temp << data[0];
            str = replace(temp.str(), "\\", "\\\\", 0, false);
            str = replace(str, "\"", "\\\"", 0, false);
            
            ss << "(\"" << str.c_str() << "\")";
            _data = ss.str();
        } else {
            _data = "()";
        }
        return;
    }
    
    // For remove unnecessary space (condition in a loop may take a lot of time)
    ss << "(";
    ss << '"' << data[0] << '"';
    for (size_t i = 1; i < data.size(); i++) {
        temp.str("");
        temp.clear();
        temp << data[i];
        str = replace(temp.str(), "\\", "\\\\", 0, false);
        str = replace(str, "\"", "\\\"", 0, false);
        
        ss << ", \"" << str.c_str() << '"';
    }
    ss << ")";
    
    _data = ss.str();
}

std::string Variant::replace(std::string str, std::string from, std::string to, size_t n, bool replaceAllMatches)
{
    size_t find;
    size_t replaced = (n == 0) ? 1 : 0;
    
    if (!replaceAllMatches) {
        size_t pos = 0;
        while ((find = str.find(from, pos)) != std::string::npos && replaced != n) {
            str.erase(find, from.length());
            str.insert(find, to);
            pos = find + to.length();
            replaced++;
        }
        return str;
    }
    
    // Replace all matches.
    while ((find = str.find(from)) != std::string::npos && replaced != n) {
        str.erase(find, from.length());
        str.insert(find, to);
        replaced++;
    }
    
    return str;
}

Variant::Variant()
{
    _data = "";
}


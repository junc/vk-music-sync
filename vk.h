#ifndef VK_H
#define VK_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <jansson.h>
#include <curl/curl.h>

#include "depends/variant.h"

class VK
{
    std::string _curlData;
    std::string _token;
    std::string _lastError;
    std::string _url;
    
    static size_t writeCallback(char* buf, size_t size, size_t nmemb, void* userp);
    static size_t downloadCallback(void *buf, size_t size, size_t nmemb, FILE *stream);
    static int progress_func(void* ptr, double totalToDownload, double nowDownloaded, double totalToUpload, double nowUploaded);
    json_error_t json_error;
    
    json_t* j_root;
    json_t* j_error;
    json_t* j_object;
    json_t* j_array;
    json_t* j_element;
    
    void checkError();
    
public:
    static bool VERBOSE;
    static ushort WIDTH_PROGRESS_BAR;
    
    struct Music
    {
        std::string artist;
        std::string title;
        std::string url;
        std::string lyrics_id;
        std::string id;
        size_t      duration;
    };
    
    const char* VK_API_VERSION = "5.24";
    const char* USER_AGENT = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/36.0.1985.143 Safari/537.36";
    
    std::string user;    // number or word
    std::string user_id; // only number
    std::string first_name;
    std::string last_name;
    
    size_t count;
    size_t batch;
    std::vector<Music> musicList;
    
    VK();
    VK(std::string token);
    
    void setToken(const std::string &token);
    bool request(const char* method, const char* params, long timeout = 30);
    bool download(const char* url, const char* output);
    
    void getUserInfo(const char* json);
    void getUserMusic(const char* json);
    
    const char* getData();
    const char* getLastError();
    const char* getLastURL();
};

#endif // VK_H

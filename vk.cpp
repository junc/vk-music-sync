#include "vk.h"

std::string vk_curl_data_pub = "";

bool VK::VERBOSE = false;
unsigned short VK::WIDTH_PROGRESS_BAR = 50;

size_t VK::writeCallback(char* buf, size_t size, size_t nmemb, void* /* userp */)
{
    for (size_t i = 0; i < size * nmemb; i++) {
        vk_curl_data_pub.push_back(buf[i]);
    }
    return size * nmemb;
}

size_t VK::downloadCallback(void *buf, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(buf, size, nmemb, stream);
    return written;
}

int VK::progress_func(void* /* ptr */, double totalToDownload, double nowDownloaded, double /* totalToUpload */, double /* nowUploaded */)
{
    double fract = nowDownloaded / totalToDownload;
    size_t count = static_cast<size_t>(fract * WIDTH_PROGRESS_BAR);
    
    if (count > WIDTH_PROGRESS_BAR) {
        return 0;
    }
    
    printf("[");
    
    for (size_t i = 0; i < count; i++) {
        putchar('#');
    }
    
    for (size_t i = 0; i < WIDTH_PROGRESS_BAR - count; i++) {
        putchar('-');
    }
    
	printf("] %lu%%", static_cast<size_t>(fract * 100));
    
    putchar('\r');
    fflush(stdout);
    
    return 0;
}

void VK::checkError()
{
    if (!j_root) {
        fprintf(stderr, "Error: json parse error: %s\n", json_error.text);
        exit(1);
    }
    
    j_error = json_object_get(j_root, "error");
    if (json_is_object(j_error)){
        j_element = json_object_get(j_error, "error_msg");
        
        if (j_element) {
            fprintf(stderr, "Error: VK error response: %s\n", json_string_value(j_element));
        } else {
            fprintf(stderr, "Error: Error retrieving error in VK::checkError().\n");
        }
        exit(1);
    }
}

VK::VK() : count(100), batch(200)
{
    curl_global_init(CURL_GLOBAL_ALL);
}

VK::VK(std::string token) : count(100), batch(200)
{
    curl_global_init(CURL_GLOBAL_ALL);
    setToken(token);
}

void VK::setToken(const std::string &token)
{
    _token = token;
}

/**
 * Return true if somithing is wrong.
 * @return bool
 */
bool VK::request(const char* method, const char* params, long timeout)
{
    if (_token.empty()) {
        _lastError = "Token is not set.";
        return true;
    }
    
    _url = std::string("https://api.vk.com/method/") + method + "?" + params + "&access_token=" + _token + "&v=" + VK_API_VERSION;
    
    if (VERBOSE) {
        printf("\nURL: %s\n", _url.c_str());
    }
    
    CURLcode code(CURLE_FAILED_INIT);
    CURL* curl = curl_easy_init();
    
    vk_curl_data_pub = "";
    
    if (curl) {
        if (   CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &VK::writeCallback))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_VERBOSE, 0))
			&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_URL, _url.c_str()))
			&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT))
			&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ""))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_COOKIELIST, "ALL"))
        ) {
            code = curl_easy_perform(curl);
            _curlData = vk_curl_data_pub;
            
            if (VERBOSE) {
                printf("Response: %s\n\n", _curlData.c_str());
            }
        }
        
        curl_easy_cleanup(curl);
    }
    
    if (code != CURLE_OK) {
        _lastError = curl_easy_strerror(code);
    }
    
    return code != CURLE_OK;
}

bool VK::download(const char *url, const char *output)
{
    CURLcode code(CURLE_FAILED_INIT);
    CURL* curl = curl_easy_init();
    FILE *fp;
    
    long http_code = 0;
    
    if (curl) {
		
#ifdef _WIN32
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
		std::wstring newStr = convert.from_bytes(output);
		fp = _wfopen(newStr.c_str(), L"wb");
#else
		fp = fopen(output, "wb");
#endif
        
        if (   CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &VK::downloadCallback))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, &VK::progress_func))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_VERBOSE, 0))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 900L))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_URL, url))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L))
			&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT))
			&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ""))
            && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_COOKIELIST, "ALL"))
        ) {
            code = curl_easy_perform(curl);
            
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            
            if (http_code != 200 || code == CURLE_ABORTED_BY_CALLBACK) {
                // Erase last progress bar.
                printf("\r");
                for (int i = 0; i < 80; i++) {
                    printf(" ");
                }
                printf("\r");
                fflush(stdout);
                
                curl_easy_cleanup(curl);
                fclose(fp);
                
                _lastError = "Response status: " + Variant(http_code).toString();
                return true;
            } else {
                printf("\n"); // After progress bar
            }
        }
        
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    
    if (code != CURLE_OK) {
        _lastError = curl_easy_strerror(code);
    }
    
    return code != CURLE_OK;
}

/**
 * Return true if somithing is wrong.
 * @return bool
 */
void VK::getUserInfo(const char* json)
{
    j_root = json_loads(json, 0, &json_error);
    checkError();
    
    j_array = json_object_get(j_root, "response");
    if (!j_array || !json_is_array(j_array)) {
        fprintf(stderr, "Error: json:/response\n");
        exit(1);
    }
    
    j_object = json_array_get(j_array, 0);
    if (!j_object || !json_is_object(j_object)) {
        fprintf(stderr, "Error: json:/response/element\n");
        exit(1);
    }
    
    user_id    = Variant(json_integer_value(json_object_get(j_object, "id"))).toString();
    first_name = json_string_value(json_object_get(j_object, "first_name"));
    last_name  = json_string_value(json_object_get(j_object, "last_name"));
}

void VK::getUserMusic(const char *json)
{
    size_t i = 0;
    j_root = json_loads(json, 0, &json_error);
    checkError();
    
    j_object = json_object_get(j_root, "response");
    if (!j_object || !json_is_object(j_object)) {
        fprintf(stderr, "Error: json:/response\n");
        exit(1);
    }
    
    count = json_integer_value(json_object_get(j_object, "count"));
    
    j_array = json_object_get(j_object, "items");
    if (!j_array || !json_is_array(j_array)) {
        fprintf(stderr, "Error: json:/response/items\n");
        exit(1);
    }
    
    json_array_foreach(j_array, i, j_object) {
        Music music;
        music.artist    = json_string_value(json_object_get(j_object, "artist"));
        music.title     = json_string_value(json_object_get(j_object, "title"));
        music.url       = json_string_value(json_object_get(j_object, "url"));
        music.lyrics_id = Variant(json_integer_value(json_object_get(j_object, "lyrics_id"))).toString();
        music.id        = Variant(json_integer_value(json_object_get(j_object, "id"))).toString();
        music.duration  = json_integer_value(json_object_get(j_object, "duration"));
        
        
        musicList.push_back(music);
    }
}

const char* VK::getData()
{
    return _curlData.c_str();
}

const char* VK::getLastError()
{
    return _lastError.c_str();
}

const char *VK::getLastURL()
{
    return _url.c_str();
}

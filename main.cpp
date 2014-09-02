#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <shlobj.h>
#include <codecvt>
#else
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#endif

#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/fileref.h>

#include "depends/iniconfig.h"
#include "depends/variant.h"
#include "vk.h"

#ifdef _WIN32
    std::string homedir;
    const char DS = '\\';
#else
    // User's directory.
    struct passwd *pw = getpwuid(getuid());
    std::string homedir = pw->pw_dir;
    const char DS = '/';
#endif

// Dir
struct stat sb;

const char* VERSION = "0.0.4";

int makeDir(const char* dirname);
bool dirExists(const char* dirname);
void help();
void checkArgument(const int &cur, const int &all, const char* msg);
short checkVersions(std::string ver1, std::string ver2);
std::vector<std::string> GetFilesInDirectory(const std::string &directory);

#ifdef _WIN32
void sleep(size_t sec) { Sleep(sec * 1000); }
#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
    setlocale(LC_ALL, "Russian");
    char homedir_path[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, homedir_path) == S_OK) {
        homedir = homedir_path;
    }
#endif

    std::string programDir;
    std::string configFilepath;

    IniConfig config;
    VK vk;
    bool error = false;
    bool save  = false;
    bool saveMeta  = true;
    bool isRemove  = true;

    size_t currentBatch = 0;
    std::string request;
    
    std::stringstream log;
    std::string filename;
    std::string filepath;
    std::string dist;
    
    std::vector<std::string> cloudFiles;
    std::vector<std::string> localFiles;

#ifdef __linux__
    programDir = homedir + "/.config/vk-music-sync";
#elif _WIN32
    programDir = homedir + "\\Documents\\Configs\\vk-music-sync";
#else
    programDir = homedir + "/.config/vk-music-sync";
#endif

    // Create program's dir if not exist.
    if (!dirExists(programDir.c_str())) {
        printf("Creating directory: %s\n", programDir.c_str());
        int status = makeDir(programDir.c_str());

        if (status != 0) {
            fprintf(stderr, "Error: Directory not exist and we can't create one.\n");
            exit(1);
        }
    }

    // Configuration
    configFilepath = programDir + DS + "config.ini";
    config.setConfig(configFilepath.c_str());
    config.parse();

    // If just created
    if (config.isCreated() || config.get("Settings", "version", "").isEmpty()
          || checkVersions(VERSION, config.get("Settings", "version", "").toCharPointer()) == 1
        ) {
        config.get("Settings", "token",   "");
        config.get("Settings", "dist",    ".");
        config.get("Settings", "user_id", "");
        config.set("Settings", "version", VERSION);
        config.save();
    }

    // Parsing arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-t" || arg == "--token") {
            checkArgument(i, argc, arg.c_str());
            config.set("Settings", "token", argv[++i]);
            save = true;
        }
        else if (arg == "-u" || arg == "--user") {
            checkArgument(i, argc, arg.c_str());
            vk.user = argv[++i];
        }
        else if (arg == "-ui" || arg == "--user-id") {
            checkArgument(i, argc, arg.c_str());
            vk.user_id = argv[++i];
        }
        else if (arg == "-d" || arg == "--dir") {
            checkArgument(i, argc, arg.c_str());
            config.set("Settings", "dist", argv[++i]);
        }
        else if (arg == "-h" || arg == "--help") {
            help();
            exit(0);
        }
        else if (arg == "-nd" || arg == "--no-delete") {
            isRemove = false;
        }
        else if (arg == "--no-save-meta") {
            saveMeta = false;
        }
        else if (arg == "--reset") {
            std::string token = config.get("Settings", "token", "").toString();
            config.reset();
            config.set("Settings", "token", token);
            config.save();
            printf("Settings are cleared due.\n");
            exit(0);
        }
        else if (arg == "--debug") {
            VK::VERBOSE = true;
        }
        else if (arg == "-v" || arg == "--version") {
            printf("%s\n", VERSION);
            exit(0);
        }
        else if (arg == "-s" || arg == "--save") {
            save = true;
        } else {
            fprintf(stderr, "Error: Invalid option '%s'.\n", argv[i]);
            exit(1);
        }
    }

    if (VK::VERBOSE) {
        curl_version_info_data* vinfo = curl_version_info(CURLVERSION_NOW);

        if (vinfo->features & CURL_VERSION_SSL){
            std::cout << "CURL: SSL enabled\n";
        } else {
            std::cout << "CURL: SSL not enabled\n";
        }
    }

    vk.setToken(config.get("Settings", "token").toCharPointer());

    if (!config.get("Settings", "user_id").isEmpty()) {
        vk.user_id = config.get("Settings", "user_id").toString();
    }

    // By user name
    if (!vk.user.empty()) {
        error = vk.request("users.get", (std::string("user_ids=") + vk.user).c_str());
    }
    // By user ID
    else if (!vk.user_id.empty()) {
        error = vk.request("users.get", (std::string("user_ids=") + vk.user_id).c_str());
    }
    // Get owner ID of token
    else {
        error = vk.request("users.get", "");
    }

    if (error) {
        fprintf(stderr, "Error: %s\n", vk.getLastError());
        exit(1);
    }

    vk.getUserInfo(vk.getData());

    if (vk.user_id.empty()) {
        fprintf(stderr, "Error: Could not get User ID.\n");
        exit(1);
    }

    config.set("Settings", "user_id", vk.user_id);
    printf("User: %s %s (id%s)\n", vk.first_name.c_str(), vk.last_name.c_str(), vk.user_id.c_str());
    
    dist = config.get("Settings", "dist").toString();
    dist = Variant::replace(dist, std::string(2, DS), std::string(1, DS));
    while (dist.back() == DS) {
        dist.erase(dist.length()-1);
    }
    config.set("Settings", "dist", dist);
    
    if (save) {
        config.save();
    }

    // Get music
    while (currentBatch * vk.batch < vk.count) {
        if (currentBatch) {
            size_t partBegin = currentBatch * vk.batch;
            size_t partEnd = partBegin + vk.batch;

            printf("Sleep 3 seconds...\n");
            printf("Getting %lu-%lu of %lu.\n", partBegin, (partEnd > vk.count ? vk.count : partEnd), vk.count);
            sleep(3);
        }

        request = std::string("owner_id=") + vk.user_id
                + "&count="  + Variant(vk.batch).toString()
                + "&offset=" + Variant(currentBatch * vk.batch).toString();

        error = vk.request("audio.get", request.c_str());
        if (error) {
            fprintf(stderr, "Error: %s\n", vk.getLastError());
            exit(1);
        }

        // std::cout << vk.getData() << '\n';
        vk.getUserMusic(vk.getData());

        currentBatch++;
    }
    
    if (config.get("Settings", "dist").isEmpty()) {
        config.set("Settings", "dist", std::string("."));
    }
    
    config.set("Settings", "dist", config.get("Settings", "dist").toString() + DS + vk.first_name + " " + vk.last_name);
    
    // Create music directory
    if (!dirExists((config.get("Settings", "dist").toString() + DS).c_str())) {
        printf("Creating directory: %s\n", (config.get("Settings", "dist").toString() + DS).c_str());
        int status = makeDir((config.get("Settings", "dist").toString() + DS).c_str());

        if (status != 0) {
            fprintf(stderr, "Error: Could not create music directory.\n");
            exit(1);
        }
    }

    int attempts = 0;
    localFiles = GetFilesInDirectory(config.get("Settings", "dist").toCharPointer());

    // Check the same titles
    for (size_t i = 0; i < vk.musicList.size(); i++) {
        filename = vk.musicList[i].artist + " - " + vk.musicList[i].title;
        bool found = false;

        for (size_t y = i+1; y < vk.musicList.size(); y++) {
            if (vk.musicList[i].artist == vk.musicList[y].artist
                && vk.musicList[i].title == vk.musicList[y].title
            ) {
                // Rename
                found = true;
                vk.musicList[y].title += " (id";
                vk.musicList[y].title += vk.musicList[y].id;
                vk.musicList[y].title += ")";
            }
        }

        if (found) {
            vk.musicList[i].title += " (id";
            vk.musicList[i].title += vk.musicList[i].id;
            vk.musicList[i].title += ")";
        }
    }

    // Downloading...
    printf("Directory: %s\n", (config.get("Settings", "dist").toString() + DS).c_str());
    for (size_t i = 0; i < vk.musicList.size(); i++) {
        filename = vk.musicList[i].artist + " - " + vk.musicList[i].title + ".mp3";
        filepath = config.get("Settings", "dist").toString() + '/' + filename;

        cloudFiles.push_back(filepath);

        if (std::find(localFiles.begin(), localFiles.end(), filepath) != localFiles.end()) {
            //  printf("Skip file '%s'\n", filename.c_str());
            continue;
        }

        if (!attempts) {
            std::cout << std::endl;
            // printf("Sleep 1 second...\n");
            sleep(1);
        }

#ifdef _WIN32
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
        std::wstring filename_w = convert.from_bytes(filename.c_str());
        std::wcout << L"Downloading '" << filename_w << L"'\n";
#else
        printf("Downloading '%s'\n", filename.c_str());
#endif
        
        error = vk.download(vk.musicList[i].url.c_str(), filepath.c_str());
        if (!error) {
            attempts = 0;

            // Save metadata
            if (saveMeta) {
                TagLib::MPEG::File file(filepath.c_str());

                log << "\nDownloaded: " << filename.c_str();

                if (file.isValid() && file.ID3v2Tag()) {
                    file.ID3v2Tag()->setArtist(TagLib::String(vk.musicList[i].artist.c_str(), TagLib::String::UTF8));
                    file.ID3v2Tag()->setTitle(TagLib::String(vk.musicList[i].title.c_str(), TagLib::String::UTF8));
                    file.save();
                }
            }

        } else {
            fprintf(stderr, "Error: %s\n\n", vk.getLastError());
            printf("Sleep 7 seconds...\n");
            sleep(7);

            if (attempts > 14) {
                continue;
            }

            printf("Try again...\n");
            i--;
            attempts++;
        }
    }

    if (log.str().length()) {
#ifdef _WIN32
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
        std::wstring log_w = convert.from_bytes(log.str().c_str());
        std::wcout << log_w << '\n';
#else
        std::cout << log.str() << '\n';
#endif
    } else {
        std::cout << "Nothing to do.\n";
    }

    // Remove other files
    if (isRemove) {
        for (size_t i = 0; i < localFiles.size(); i++) {
            if (std::find(cloudFiles.begin(), cloudFiles.end(), localFiles[i]) == cloudFiles.end()) {
                if (remove(localFiles[i].c_str()) != 0) {
                    fprintf(stderr, "Error deleting file: %s\n", localFiles[i].c_str());
                } else {
                    fprintf(stderr, "Removed: %s\n", localFiles[i].c_str());
                }
            }
        }
    }
    
    return 0;
}

// Functions

void help()
{
    const char* programName = "vk-music-sync";
    
    printf("VK Music Sync.\n");
    printf("Description:\n");
    printf("  Synchronize your VK playlist with your local storage. Program needs 'token' for requests to VK.\n");
    printf("\nOptions:\n");
    printf("  -t,  --token <TOKEN>   Access token. It needs for get music from your profile.\n");
    printf("  -u,  --user <USER>     User ID or user name like vk.com/durov.\n");
    printf("  -ui, --user-id <ID>    User ID.\n");
    printf("  -d,  --dir <DIR>       Output dir.\n");
    printf("  -s,  --save            Save user ID and other to config file.\n");
    printf("  -nd, --no-delete       Don't delete files in <DIR>.\n");
    printf("  --reset                Reset config file (excluding token).\n");
    printf("  --no-save-meta         Metadata will not save.\n");
    printf("  -v,  --version         Show version and exit.\n");
    printf("\nExamples:\n");
    printf("  %s\n", programName);
    printf("  %s --token <TOKEN>\n", programName);
    printf("  %s --user durov\n", programName);
    printf("  %s --user-id 1 --save\n", programName);
    printf("  %s --dir ~/Music\n", programName);
    printf("\nFor get token, you can go to vk: %s and copy 'token' from URL.\n",
           "https://oauth.vk.com/authorize?client_id=4509223&scope=audio&redirect_uri=http:%2F%2Foauth.vk.com%2Fblank.html&display=wap&response_type=token");
    printf("\nVersion: %s\n", VERSION);
    printf("Source code: https://github.com/junc/vk-music-sync\n");
    printf("Enjoy.\n");
}

bool dirExists(const char* dirname)
{
    bool exists = false;
#ifdef _WIN32
    DWORD ftyp = GetFileAttributesA(dirname);
    if (ftyp != INVALID_FILE_ATTRIBUTES && ftyp & FILE_ATTRIBUTE_DIRECTORY) {
        exists = true;
    }
#else
    if (stat(dirname, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        exists = true;
    }
#endif
    return exists;
}

int makeDir(const char* dirname)
{
    int result = 1;
#ifdef _WIN32
    result = system((std::string("md \"") + Variant::replace(dirname, "\"", "\\\"") + '"').c_str());
    if (result == 1) {
        result = 0;
    }
#else
    char cmd[255] = {0};
    sprintf(cmd, "mkdir -p %s", Variant::replace(dirname, " ", "\\ ", 0, false).c_str());
    result = system(cmd);
    
    // result = mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
    return result;
}

void checkArgument(const int &cur, const int &all, const char* msg)
{
    if (cur+1 >= all) {
        fprintf(stderr, "Error: Option '%s' requires an argument.\n", msg);
        exit(1);
    }
}

short checkVersions(std::string ver1, std::string ver2)
{
    if (ver1 == ver2) {
        return 0;
    }
    
    if (ver1.empty()) {
        return -1;
    }
    
    if (ver2.empty()) {
        return 1;
    }
    
    std::stringstream ss;
    std::string token;
    std::vector<std::string> tokens1;
    std::vector<std::string> tokens2;
    
    ss << ver1;
    while (std::getline(ss, token, '.')) {
        tokens1.push_back(token);
    }
    
    ss.str("");
    ss.clear();
    
    ss << ver2;
    while (std::getline(ss, token, '.')) {
        tokens2.push_back(token);
    }
    
    size_t size = tokens1.size() > tokens2.size() ? tokens2.size() : tokens1.size();
    for (size_t i = 0; i < size; i++) {
        if (tokens1[i] > tokens2[i]) {
            return 1;
        } else if (tokens1[i] < tokens2[i]) {
            return -1;
        }
    }
    
    if (tokens1.size() > tokens2.size()) {
        return 1;
    } else if (tokens1.size() < tokens2.size()) {
        return -1;
    }
    
    return 0;
}

#ifdef _WIN32
std::string utf8_encode(const std::wstring &wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte (CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
#endif

std::vector<std::string> GetFilesInDirectory(const std::string &directory)
{
    std::vector<std::string> out;
#ifdef _WIN32
    
    HANDLE dir;
    WIN32_FIND_DATA file_data;

    std::string strdir = directory + "/*";
    wchar_t filename[4096] = {0};
    const char* dirname = strdir.c_str();
    MultiByteToWideChar(0, 0, dirname, strlen(dirname), filename, strlen(dirname));

    if ((dir = FindFirstFile(filename, &file_data)) == INVALID_HANDLE_VALUE)
        return out;

    do {
        std::string file_name = utf8_encode(file_data.cFileName);
        std::string full_file_name = directory + "/" + file_name;
        const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (file_name[0] == '.')
            continue;

        if (is_directory)
            continue;

        out.push_back(full_file_name);
    } while (FindNextFile(dir, &file_data));

    FindClose(dir);
#else
    DIR *dir;
    class dirent *ent;
    class stat st;
    
    dir = opendir(directory.c_str());
    while ((ent = readdir(dir)) != NULL) {
        const std::string file_name = ent->d_name;
        const std::string full_file_name = directory + "/" + file_name;
        
        if (file_name[0] == '.')
            continue;

        if (stat(full_file_name.c_str(), &st) == -1)
            continue;

        const bool is_directory = (st.st_mode & S_IFDIR) != 0;

        if (is_directory)
            continue;
        
        out.push_back(full_file_name);
    }
    closedir(dir);
#endif
    
    return out;
}

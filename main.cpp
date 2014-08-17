#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <algorithm>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/fileref.h>

#include "depends/iniconfig.h"
#include "depends/variant.h"
#include "vk.h"

// User's directory.
struct passwd *pw = getpwuid(getuid());
std::string homedir = pw->pw_dir;

// Dir
struct stat sb;

const char* VERSION = "0.0.1";

#ifdef _WIN32
    const char DS = '\\';
#else
    const char DS = '/';
#endif

void help();
void checkArgument(const int &cur, const int &all, const char* msg);
short checkVersions(std::string ver1, std::string ver2);
std::vector<std::string> GetFilesInDirectory(const std::string &directory);

int main(int argc, char **argv)
{
    std::string programDir;
    std::string configFilepath;
    
    IniConfig config;
    VK vk;
    bool error = false;
    bool save  = false;
    bool saveMeta  = true;
    
    size_t currentBatch = 0;
    std::string request;
    
    std::string filename;
    std::string filepath;
    std::vector<std::string> cloudFiles;
    std::vector<std::string> localFiles;
    
#ifdef __linux__ 
    programDir = homedir + "/.config/vk-music-sync";
#elif _WIN32
    programDir = homedir + "/Configs/vk-music-sync";
#else
    programDir = homedir + "/.config/vk-music-sync";
#endif
    
    // Create program's dir if not exist.
    if (stat(programDir.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        printf("Creating directory: %s\n", programDir.c_str());
        int status = mkdir(programDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        
        if (status != 0) {
            fprintf(stderr, "Error: Directory not exist and we can't create one.\n");
            exit(1);
        }
    }
    
    // Configuration
    configFilepath = programDir + "/config.ini";
    config.setConfig(configFilepath.c_str());
    config.parse();
    
    // If just created
    if (config.isCreated() || config.get("Settings", "version", "").isEmpty()
          || checkVersions(VERSION, config.get("Settings", "version", "").toCharPointer()) == 1
        ) {
        std::cout << "hello\n";
        config.get("Settings", "token",   "");
        config.get("Settings", "dist",    "./");
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
        else if (arg == "--no-save-meta") {
            saveMeta = false;
        }
        else if (arg == "--reset") {
            config.reset();
            printf("Settings are cleared due.\n");
            exit(0);
        }
        else if (arg == "--debug") {
            VK::VERBOSE = true;
        }
        else if (arg == "-s" || arg == "--save") {
            save = true;
        } else {
            fprintf(stderr, "Error: Invalid option '%s'.\n", argv[i]);
            exit(1);
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
        // config.set("Settings", "dist", homedir + DS + "Music");
        config.set("Settings", "dist", std::string("."));
    }
    
    config.set("Settings", "dist", config.get("Settings", "dist").toString() + DS + vk.first_name + " " + vk.last_name);
    
    // Create music directory
    if (stat((config.get("Settings", "dist").toString() + DS).c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        printf("Creating directory: %s\n", (config.get("Settings", "dist").toString() + DS).c_str());
        int status = mkdir((config.get("Settings", "dist").toString() + DS).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        
        if (status != 0) {
            fprintf(stderr, "Error: Could not create music directory.\n");
            exit(1);
        }
    }
    
    int attempts = 0;
    localFiles = GetFilesInDirectory(config.get("Settings", "dist").toCharPointer());
    
    // Downloading...
    printf("Directory: %s\n", (config.get("Settings", "dist").toString() + DS).c_str());
    for (size_t i = 0; i < vk.musicList.size(); i++) {
        filename = vk.musicList[i].artist + " - " + vk.musicList[i].title + ".mp3";
        filepath = config.get("Settings", "dist").toString() + DS + filename;
        
        cloudFiles.push_back(filepath);
        
        if (std::find(localFiles.begin(), localFiles.end(), filepath) != localFiles.end()) {
            printf("Skip file '%s'\n", filename.c_str());
            continue;
        }
        
        if (!attempts) {
            printf("\nSleep 3 seconds...\n");
            sleep(3);
        }
        
        printf("Downloading '%s'\n", filename.c_str());
        
        error = vk.download(vk.musicList[i].url.c_str(), filepath.c_str());
        if (!error) {
            attempts = 0;
            
            // Save metadata
            if (saveMeta) {
                TagLib::MPEG::File file(filepath.c_str());
                
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
    
    // Remove other files
    for (size_t i = 0; i < localFiles.size(); i++) {
        if (std::find(cloudFiles.begin(), cloudFiles.end(), localFiles[i]) == cloudFiles.end()) {
            if (remove(localFiles[i].c_str()) != 0) {
                fprintf(stderr, "Error deleting file: %s\n", localFiles[i].c_str());
            } else {
                fprintf(stderr, "Removed: %s\n", localFiles[i].c_str());
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
    printf("  -t,  --token <TOKEN>   Access token. It needs for get music from your profile.\n");
    printf("  -u,  --user <USER>     User ID or user name like vk.com/durov.\n");
    printf("  -ui, --user-id <ID>    User ID.\n");
    printf("  -d,  --dir <DIR>       Output dir.\n");
    printf("  -s,  --save            Save user ID and other to config file.\n");
    printf("  --reset                Reset config file (excluding token).\n");
    printf("  --no-save-meta         Metadata will not save.\n");
    printf("Examples:\n");
    printf("  %s\n", programName);
    printf("  %s --token <TOKEN>\n", programName);
    printf("  %s --user durov\n", programName);
    printf("  %s --user-id 1 --save\n", programName);
    printf("  %s --dir ~/Music\n", programName);
    printf("For get token, you can go to vk: %s and copy 'token' from URL.\n",
           "https://oauth.vk.com/authorize?client_id=4509223&scope=audio&redirect_uri=http:%2F%2Foauth.vk.com%2Fblank.html&display=wap&response_type=token");
    printf("Source code: https://github.com/junc/vk-music-sync\n");
    printf("Enjoy.\n");
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

std::vector<std::string> GetFilesInDirectory(const std::string &directory)
{
    std::vector<std::string> out;
#ifdef WINDOWS
    
    HANDLE dir;
    WIN32_FIND_DATA file_data;

    if ((dir = FindFirstFile((directory + "/*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
        return out; /* No files found */

    do {
        const std::string file_name = file_data.cFileName;
        const std::string full_file_name = directory + "/" + file_name;
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

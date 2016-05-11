
VK Music Sync
=============

Synchronize music with your VK profile.
If you have added some music on your VK profile, vk-music-sync will download missing files. If you delete some music, program delete those files in your local folder.
vk-music-sync writes metadata `artist` and `title` to mp3 files.

![VK Music Sync](https://dl.dropboxusercontent.com/u/110426823/projects/scr/vk-music-sync.png "VK Music Sync")

To use vk-music-sync you need to [get **token**](#how-to-get-token) from VK.

If you have any trouble or if you have found any mistake contact me. `Thank you`

## Downloads
+ [Windows x86](https://dl.dropboxusercontent.com/u/110426823/projects/binaries/vk-music-sync-0.0.4.7z)

## Dependencies
**vk-music-sync** needs:
- libcurl (with SSL)
- jansson
- taglib

### Install for Debian
    ~$ sudo apt-get install libjansson-dev
    ~$ sudo apt-get install libcurl4-openssl-dev

## Compiling
### On Linux
    $ git clone https://github.com/junc/vk-music-sync.git
    $ cd vk-music-sync
    $ make
### On Windows
You must have compiled [dependencies](#dependencies). After, you can build vk-music-sync. If you use Visual Studio (or other IDE), you must add libraries and include paths to the project.

## How to get token
In order to get token, you have to go **[to vk](https://oauth.vk.com/authorize?client_id=4509223&scope=audio,offline&redirect_uri=http:%2F%2Foauth.vk.com%2Fblank.html&display=wap&response_type=token)**, confirm the action and copy 'access_token' from URL.

### First run
You have must save *token* and *directory* in order to not to repeat again in next time:

    $ ./vk-music-sync --token <your_token> --dir ~/Music --save

Program will save settings.

#### Examples
You can use `--user` or `-u` option. This option needs a user ID or user name like `durov`.

    $ ./vk-music-sync
    $ ./vk-music-sync --user <user_id>
    $ ./vk-music-sync --no-save-meta
    $ ./vk-music-sync --help # for more info

Enjoy :smirk:

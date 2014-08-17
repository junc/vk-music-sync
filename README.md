
VK Music Sync
=============

Synchronize music from your VK profile.
If you have added some music on your VK profile, vk-music-sync will download missing files. If you delete some music, program delete those files in your local folder.
vk-music-sync writes metadata `artist` and `title` to mp3 files.

For use vk-music-sync you need to get **token** from VK.

If you have any trouble or if you have found any mistake contact me. `Thank you`

## Dependencies
**vk-music-sync** needs:
- libcurl
- jansson
- taglib with id3v2

### Compiling
    $ git clone https://github.com/junc/vk-music-sync.git
    $ cd vk-music-sync
    $ make

## How to get *token*
For get token, you have to go **[to vk](https://oauth.vk.com/authorize?client_id=4509223&scope=audio&redirect_uri=http:%2F%2Foauth.vk.com%2Fblank.html&display=wap&response_type=token)**, confirm the action and copy 'access_token' from URL.

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

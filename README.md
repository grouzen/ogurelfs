 Ogurelfs
================================================================================

FUSE filesystem for searching and playing music from various
online music databases like lastfm.

Concept and idea
--------------------------------------------------------------------------------

Main idea is work with online music databases in terms of regular 
files operations and be independent from music players which support
online searching. Like a bonus - accumulation music on your disk. 

Search
--------------------------------------------------------------------------------

Search is initiated by directory or file creation on specific meta file system hierarchy:
`
/artists
/albums
/tracks
`

1. Searching artist
    - `mkdir artists/Artist` will search artist(s) and if finds will create directory
       `Artist`, directories for albums `Artist/Album1, Artist/AlbumN`, and
       files of tracks inside albums directories.
2. Searching album
    - `mkdir albums/Album` like a search of artist, but it will create directories
       inside artists/ too.
3. Searching track of artist
    - `mkdir tracks/Track` is same as searhing album, but it will create directories
       inside albums/ and artists/

Results of all search queries will be stored on `ogureldb` directory,
for example on `~/ogureldb/`.


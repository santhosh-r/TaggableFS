# TaggableFS

A tags based filesystem implemented using FUSE and SQLite. TaggableFS was created to explore how a tag based filesystem might work and how it could interoperate with Linux which uses a hierarchical filesystem. There are two modes in which the filesystem can be mounted: a default mode and a tag view mode. In the default mode, a user can copy files into the filesystem and can later perform tagging operations via command line queries. In the tag view mode, the user can browse the tagged files through their tags represented as folders and also create and move tags around while the files themselves are read only.

This software was created as part of the course CSI 680 "Computer Science Research" to fulfill the project requirement for my MS degree in Computer Science at UAlbany. The idea to create a tag based filesystem was suggested to me by my project supervisor Michael Phipps.

For documentation, see https://santhosh-r.github.io/TaggableFS/

## Command Line Interface:

      bash run_tfs.sh [--tag-view] [--log]
      /tfs.out COMMAND

## Screenshots

![Default mode](images/default.png?raw=true)

![Tag view mode](images/tag_view.png?raw=true)

## Commands:
      --help
            display help.

      --log
            log messages to ROOT_DIRECTORY/metadata/log.txt.

      --tag-view
            open filesystem in read-only mode to browse tags.

      --init MOUNT_POINT ROOT_DIRECTORY
            launch daemon and mount FUSE filesystem to the given mount
            point and files are stored in root directory.

      --shutdown
            unmount FUSE filesystem and shutdown daemon.

      --tag MOUNTED_PATH TAG
            tag the file referenced by mounted path (not in tag view) with the
            given tag which will be created if not found. If the path refers to
            a folder, all files in it are tagged (non-recursive).

      --untag MOUNTED_PATH TAG
            untag the file referenced by mounted path (not in tag view) if
            tagged with the given tag. If the path refers to a folder, all files
            in it are untagged (non-recursive).

      --nest TAG PARENT_TAG
            nest the given tag inside the given parent tag if both are valid.

      --unnest TAG PARENT_TAG
            unnest the given tag from the given parent tag if both are valid.

      --stats
            display stats regarding mounted FUSE filesystem.

      --search-tags TAG_1 TAG_2 ... TAG_N [--strict]
            search for tagged files with any of the given tags
            or with all of them if --strict option is used.

      --create-tag TAG
            create tag with no children.

      --delete-tag TAG
            delete tag if it has no children.

      --get-tags FILE_PATH
            display all tags current used to tag the file.

## References
1. Practical File System Design - Dominic Giampaolo
2. [Writing a FUSE Filesystem: a Tutorial](https://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/) - Prof. Joseph J. Pfeiffer
3. [cppreference.com](https://en.cppreference.com/w/) for C++
4. man pages for POSIX filesystem functions and for POSIX message queues.
5. Using SQLite - Jay A. Kreibich
6. [sqlite.org](https://www.sqlite.org/) for SQLite
7. Various Q&As on [Stack Overflow](https://stackoverflow.com) for C++, POSIX message queues and SQLite


/**
 * @file FUSEFileSystem.hpp
 * @author Santhosh Ranganathan
 * @brief The header file for the FUSEFileSystem class.
 *
 * @details This file contains the class definition for the FUSEFileSystem
 * class. The FUSEFileSystem class handles initializing and shutting down of
 * the FUSE filesystem and also communication with the TaggableFS daemon for
 * various filesystem operations.
 */

#ifndef TFS_FUSEFILESYSTEM_HPP
#define TFS_FUSEFILESYSTEM_HPP

#include "common.hpp"
#include <unistd.h>
#include <dirent.h>
#include <functional>

/** FUSE version used. */
#define FUSE_USE_VERSION    26
#include <fuse.h>

namespace TaggableFS
{

/**
 * This class handles initialization and shutdown of the FUSE filesystem and communication
 * with the TaggableFS daemon for various filesystem operations.
 */
class FUSEFileSystem
{
private:
    /** Path at which the FUSE filesystem is to be mounted. */
    std::string mountPoint;

    /** Name of the original program to be passed to fuse_main(). */
    std::string programName;

    /** Sending message queue descriptor. */
    mqd_t txMQ;

    /** Receiving message queue descriptor. */
    mqd_t rxMQ;

    /** Buffer to store messages to be sent/received. */
    char buffer[TFS_MQ_MESSAGE_SIZE];

    /** Check to make sure only one instance can be initialized. */
    static bool instancedFUSEFileSystem;

    void initMQ();
    void init();
    std::vector<std::string> queryTFS(std::string query);

public:
    /** Check to see if logging is enabled. */
    static bool loggingEnabled;

    FUSEFileSystem(std::string mountPoint, std::string programName, bool enableLogging);
};

void log(std::string text);
std::string getRealPath(std::string mountedPath, bool modify = false);
bool checkIfDirectory(std::string path);

int TFSgetattr(const char *path, struct stat *buf);
int TFStruncate(const char *file, off_t length);
int TFSopen(const char *file, struct fuse_file_info *fi);
int TFSread(const char *file, char *buf, size_t nbytes, off_t offset, struct fuse_file_info *fi);
int TFSwrite(const char *file, const char *buf, size_t n, off_t offset, struct fuse_file_info *fi);
int TFSrelease(const char *file, struct fuse_file_info *fi);
int TFSmknod(const char *file, mode_t mode, dev_t dev);
int TFSopendir(const char *dir, struct fuse_file_info *fi);
int TFSreaddir(const char *dir, void *buf, fuse_fill_dir_t filler, off_t offset,
    struct fuse_file_info *fi);
int TFSmkdir(const char *path, mode_t mode);
int TFSunlink(const char *file);
int TFSrmdir(const char *dir);
int TFSrename(const char *oldpath, const char *newpath);
int TFSutime(const char *file, struct utimbuf *ubuf);

}

#endif
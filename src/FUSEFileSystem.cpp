/**
 * @file FUSEFileSystem.cpp
 * @author Santhosh Ranganathan
 * @brief The source file for the FUSEFileSystem class.
 *
 * @details This file contains the method definitions for the FUSEFileSystem
 * class.
 */

#include "FUSEFileSystem.hpp"

namespace TaggableFS
{

bool FUSEFileSystem::instancedFUSEFileSystem = false;
bool FUSEFileSystem::loggingEnabled = false;

/** Function binding to FUSEFileSystem instance's queryTFS method for use in FUSE operations. */
std::function<std::vector<std::string> (std::string query)> queryTFS;

/**
 * Constructor for the FUSEFileSystem class.
 *
 * @param mountPoint path at which the FUSE filesystem is to be mounted.
 * @param programName name of the original program to be passed to fuse_main().
 * @param enableLogging boolean to enable or disable logging.
 */
FUSEFileSystem::FUSEFileSystem(std::string mountPoint, std::string programName, bool enableLogging)
    : mountPoint(mountPoint), programName(programName)
{
    // loggingEnabled = enableLogging; // only enable if debugging FUSE operations.
    if (instancedFUSEFileSystem == true)
    {
        return;
    }
    instancedFUSEFileSystem = true;
    TaggableFS::queryTFS = std::bind(&FUSEFileSystem::queryTFS, this, std::placeholders::_1);

    init();
}

/**
 * Initilaizes message queues to send/receive messages to and from the TaggableFS daemon. If
 * they already exist, it checks if the daemon is responding.
 */
void FUSEFileSystem::initMQ()
{
    mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = TFS_MQ_MAX_MESSAGES;
    attr.mq_msgsize = TFS_MQ_MESSAGE_SIZE;
    attr.mq_curmsgs = 0;

    txMQ = mq_open("/tfs_managermq", O_WRONLY, 0660, &attr);
    if (txMQ != -1)
    {
        rxMQ = mq_open("/tfs_fusemq", O_RDONLY, 0660, &attr);
        if (rxMQ == -1)
        {
            exit(EXIT_FAILURE);
        }

        timespec time;
        clock_gettime(CLOCK_REALTIME, &time);
        time.tv_sec += 1;
        serializeMessage("FD_TEST", buffer);
        if (mq_timedsend(txMQ, buffer, TFS_MQ_MESSAGE_SIZE, 0, &time) == -1)
        {
            exit(EXIT_FAILURE);
        }

        clock_gettime(CLOCK_REALTIME, &time);
        time.tv_sec += 1;
        if (mq_timedreceive(rxMQ, buffer, TFS_MQ_MESSAGE_SIZE, NULL, &time) == -1)
        {
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * Initializes the FUSE filesystem.
 */
void FUSEFileSystem::init()
{
    initMQ();

    if (getuid() == 0 || geteuid() == 0)
    {
        exit(EXIT_FAILURE); // not allow driver to run at root access level
    }

    fuse_operations operations = {
        .getattr = TFSgetattr,
        .mknod = TFSmknod,
        .mkdir = TFSmkdir,
        .unlink = TFSunlink,
        .rmdir = TFSrmdir,
        .rename = TFSrename,
        .truncate = TFStruncate,
        .utime = TFSutime,
        .open = TFSopen,
        .read = TFSread,
        .write = TFSwrite,
        .release = TFSrelease,
        .opendir = TFSopendir,
        .readdir = TFSreaddir,
    };

    char *argument1 = new char[programName.size() + 1];
    strcpy(argument1, programName.c_str());
    char argument2[] = "-s"; // to ensure TFS is run with a single thread
                             // as the program is thread unsafe.
    char *argument3 = new char[mountPoint.size() + 1];
    strcpy(argument3, mountPoint.c_str());
    char *arguments[] = {argument1, argument2, argument3};

    int returnValue = fuse_main(3, arguments, &operations, NULL);
    serializeMessage("FD_EXIT", buffer);
    mq_send(txMQ, buffer, TFS_MQ_MESSAGE_SIZE, 0);

    delete[] argument1;
    delete[] argument3;
    exit(returnValue);
}

/**
 * Sends query to the TaggableFS daemon and receives single or multipart response.
 *
 * @param query query to be sent.
 * @return Response from the daemon.
 */
std::vector<std::string> FUSEFileSystem::queryTFS(std::string query)
{
    serializeMessage(query.c_str(), buffer);
    mq_send(txMQ, buffer, TFS_MQ_MESSAGE_SIZE, 0);

    Message m;
    std::vector<std::string> results;
    do
    {
        mq_receive(rxMQ, buffer, TFS_MQ_MESSAGE_SIZE, NULL);
        m = deserializeMessage(buffer);
        results.push_back(std::string(m.content));
    } while (m.complete == false);

    return results;
}

/**
 * Gets the actual path to a file accessed in the mounted FUSE filesystem.
 *
 * @param mountedPath relative path to the file accessed.
 * @param modify boolean to indicate if file is being accessed to modify it.
 * @return Actual path of the file accessed.
 */
std::string getRealPath(std::string mountedPath, bool modify)
{
    std::string query = modify ? "FD_GET_PATH_WRITE " : "FD_GET_PATH ";
    std::vector<std::string> results = queryTFS(query + mountedPath);
    return results[0];
}

/**
 * Checks if a path points to a directory
 *
 * @param path relative path to a presumed folder.
 * @return Boolean value if the path refers to a directory or not.
 */
bool checkIfDirectory(std::string path)
{
    std::vector<std::string> results = queryTFS("FD_IF_DIR " + path);
    return (results[0] == "TM_TRUE");
}

/**
 * A helper function to log for debugging purposes.
 *
 * @param text text to be logged.
 */
void log(std::string text)
{
    if (FUSEFileSystem::loggingEnabled == true)
    {
        queryTFS("FD_LOG " + text);
    }
}

/**
 * getattr() filesystem operation implementation for TaggableFS.
 *
 * @param path relative path to file/folder in the mounted filesystem.
 * @param buf struct for lstat() to store values in.
 * @return Return value from lstat(), 0 if folder along with values filled in buf.
 *          If both failed, the error code for no such entity (ENOENT).
 */
int TFSgetattr(const char *path, struct stat *buf)
{
    log("_TFSgetattr_");
    if (checkIfDirectory(path) == true)
    {
        // rwxr-xr-x based on permissions noticed when running 'ls -l' on folders inside /home
        int permissions = 0755;
        buf->st_mode = S_IFDIR | permissions;
        buf->st_nlink = 2;
        return 0;
    }
    std::string realPath = getRealPath(path);
    if (getFilename(realPath) != "")
    {
        int returnValue = lstat(realPath.c_str(), buf);
        if (returnValue == -1)
        {
            log("ERROR: _TFSgetattr_ lstat() failed, errno = " + std::to_string(errno));
            return -errno;
        }
        return returnValue;
    }
    return -ENOENT;
}

/**
 * truncate() filesystem operation implementation for TaggableFS.
 *
 * @param file relative path to file in the mounted filesystem.
 * @param length length for file to be truncated to.
 * @return Return value in response to FD_TRUNCATE query to TaggableFS daemon.
 */
int TFStruncate(const char *file, off_t length)
{
    log("_TFStruncate_");
    std::vector<std::string> results;
    results = queryTFS("FD_TRUNCATE " + std::to_string(length) + "," + std::string(file));
    if (results[0] != "TM_ACK")
    {
        log("ERROR: _TFStruncate_ failed");
        errno = std::stoi(results[0]);
        return -errno;
    }
    return 0;
}

/**
 * open() filesystem operation implementation for TaggableFS.
 *
 * @param file relative path to file in the mounted filesystem.
 * @param fi information of the file opened including the file descriptor.
 * @return Return value from open() called on the actual file.
 */
int TFSopen(const char *file, struct fuse_file_info *fi)
{
    log("_TFSopen_");
    std::string pathToFile = getRealPath(file);
    int fd = -1;
    if (pathToFile != "")
    {
        fd = open(pathToFile.c_str(), fi->flags, 0777);
        fi->fh = fd;
    }
    if (fd == -1)
    {
        log("ERROR: _TFSopen_ open() failed, errno = " + std::to_string(errno));
        return -errno;
    }
    return 0;
}

/**
 * read() filesystem operation implementation for TaggableFS.
 *
 * @param file relative path to file in the mounted filesystem.
 * @param buf buffer for pread() to store data in.
 * @param nbytes read length.
 * @param offset offset from start of the file.
 * @param fi information of the file passed from TFSopen().
 * @return Return value from pread() called on the actual file.
 */
int TFSread(const char *file, char *buf, size_t nbytes, off_t offset, struct fuse_file_info *fi)
{
    log("_TFSread_");
    int returnValue = pread(fi->fh, buf, nbytes, offset);
    if (returnValue == -1)
    {
        log("ERROR: _TFSread_ pread() failed, errno = " + std::to_string(errno));
        return -errno;
    }
    return returnValue;
}

/**
 * write() filesystem operation implementation for TaggableFS.
 *
 * @param file relative path to file in the mounted filesystem.
 * @param buf buffer for pwrite() to read data from to write to file.
 * @param n write length.
 * @param offset offset from start of the file.
 * @param fi information of the file passed from TFSopen().
 * @return Return value from pwrite() called on the temporary file.
 */
int TFSwrite(const char *file, const char *buf, size_t n, off_t offset, struct fuse_file_info *fi)
{
    log("_TFSwrite_");
    if (close(fi->fh) == -1)
    {
        log("ERROR: _TFSwrite_ close() failed, errno = " + std::to_string(errno));
        return -errno;
    }
    std::string pathToFile = getRealPath(file, true) + ".WRITE";
    if (getFilename(pathToFile) == ".WRITE")
    {
        log("ERROR: _TFSwrite_ getRealPath() failed");
        return -1;
    }
    int fd = open(pathToFile.c_str(), O_CREAT | fi->flags, 0777);
    fi->fh = fd;
    if (fd == -1)
    {
        log("ERROR: _TFSwrite_ open() failed, errno = " + std::to_string(errno));
        return -errno;
    }
    int returnValue = pwrite(fi->fh, buf, n, offset);
    if (returnValue == -1)
    {
        log("ERROR: _TFSwrite_ pwrite() failed, errno = " + std::to_string(errno));
        return -errno;
    }
    return returnValue;
}

/**
 * release() filesystem operation implementation for TaggableFS. FD_UPDATE query sent to update
 * hash value if changed.
 *
 * @param file relative path to file in the mounted filesystem.
 * @param fi information of the file passed from TFSopen()/TFSwrite().
 * @return Return value from close() called on the actual file.
 */
int TFSrelease(const char *file, struct fuse_file_info *fi)
{
    log("_TFSrelease_");
    int returnValue = close(fi->fh);
    queryTFS("FD_UPDATE " + std::string(file));
    if (returnValue == -1)
    {
        log("ERROR: _TFSrelease_ close() failed, errno = " + std::to_string(errno));
        return -errno;
    }
    return returnValue;
}

/**
 * mkmod() filesystem operation implementation for TaggableFS.
 *
 * @param file relative path to file in the mounted filesystem.
 * @param mode mode to be used but only regular files created.
 * @param dev ignored.
 * @return Return value from open() or error when file already exists.
 */
int TFSmknod(const char *file, mode_t mode, dev_t dev)
{
    static int tempFileNumber = 1;

    log("_TFSmknod_");
    int fd;
    std::string pathToFile = getRealPath(file, true);
    if (pathToFile == "")
    {
        log("ERROR: _TFSmknod_ getRealPath() failed");
        return -1;
    }
    if (getFilename(pathToFile) == "")
    {
        char tempFilename[16];
        sprintf(tempFilename, "TEMP%09d", tempFileNumber++);
        queryTFS("FD_ADD_TEMP " + std::string(tempFilename) + "," + std::string(file));
        pathToFile += tempFilename;
    }
    if (S_ISREG(mode))
    {
        fd = open(pathToFile.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0777);
        if (fd >= 0)
        {
            fd = close(fd);
        }
    }
    else // for now not deal with non regular files
    {
        log("ERROR: _TFSmknod_ failed");
        return -1;
    }
    return fd;
}

/**
 * opendir() filesystem operation implementation for TaggableFS.
 *
 * @param dir relative path to folder in the mounted filesystem.
 * @param fi information of the folder opened.
 * @return 0 if folder, else -1.
 */
int TFSopendir(const char *dir, struct fuse_file_info *fi)
{
    log("_TFSopendir_");
    if (checkIfDirectory(dir) == false)
    {
        fi->fh = 1;
        log("ERROR: _TFSopendir_ failed");
        return -1;
    }
    fi->fh = 0;
    return 0;
}

/**
 * readdir() filesystem operation implementation for TaggableFS.
 *
 * @param dir relative path to folder in the mounted filesystem.
 * @param buf buffer to store contents in.
 * @param filler function to add entry in readdir() operation.
 * @param offset ignored.
 * @param fi information of the folder opened from TFSopendir().
 * @return 0 if successfully completed, else -1.
 */
int TFSreaddir(const char *dir, void *buf, fuse_fill_dir_t filler, off_t offset,
    struct fuse_file_info *fi)
{
    log("_TFSreaddir_");
    if (fi->fh != 0) // passed from TFSopendir()
    {
        log("ERROR: _TFSreaddir_ failed");
        return -1;
    }
    std::vector<std::string> contents = queryTFS("FD_READ_DIR " + std::string(dir));
    for (auto c : contents)
    {
        // Skipping . and .. for now
        if (filler(buf, c.c_str(), NULL, 0) != 0)
        {
            log("ERROR: _TFSreaddir_ filler() failed");
            return -1;
        }
    }
    return 0;
}

/**
 * rename() filesystem operation implementation for TaggableFS.
 *
 * @param oldPath relative old path to file/folder to be renamed in the mounted filesystem.
 * @param newPath relative new path to file/folder to be renamed in the mounted filesystem.
 * @return Return value in response to FD_RENAME query to TaggableFS daemon.
 */
int TFSrename(const char *oldPath, const char *newPath)
{
    log("_TFSrename_");
    std::vector<std::string> results;
    results = queryTFS("FD_RENAME " + std::string(oldPath) + "," + std::string(newPath));
    if (results[0] != "TM_ACK")
    {
        log("ERROR: _TFSrename_ failed");
        return -1;
    }
    return 0;
}

/**
 * mkdir() filesystem operation implementation for TaggableFS.
 *
 * @param path relative path to folder to be created in the mounted filesystem.
 * @param mode ignored.
 * @return Return value in response to FD_MKDIR query to TaggableFS daemon.
 */
int TFSmkdir(const char *path, mode_t mode)
{
    log("_TFSmkdir_");
    std::vector<std::string> results;
    results = queryTFS("FD_MKDIR " + std::string(path));
    if (results[0] != "TM_ACK")
    {
        log("ERROR: _TFSmkdir_ failed");
        errno = std::stoi(results[0]);
        return -errno;
    }
    return 0;
}

/**
 * unlink() filesystem operation implementation for TaggableFS.
 *
 * @param file relative path to file in the mounted filesystem.
 * @return Return value in response to FD_UNLINK query to TaggableFS daemon.
 */
int TFSunlink(const char *file)
{
    log("_TFSunlink_");
    if (checkIfDirectory(file))
    {
        log("ERROR: _TFSunlink_ failed");
        return -1;
    }
    std::vector<std::string> results;
    results = queryTFS("FD_UNLINK " + std::string(file));
    if (results[0] != "TM_ACK")
    {
        log("ERROR: _TFSunlink_ failed");
        errno = std::stoi(results[0]);
        return -errno;
    }
    return 0;
}

/**
 * rmdir() filesystem operation implementation for TaggableFS.
 *
 * @param dir relative path to folder in the mounted filesystem.
 * @return Return value in response to FD_RMDIR query to TaggableFS daemon.
 */
int TFSrmdir(const char *dir)
{
    log("_TFSrmdir_");
    std::vector<std::string> results;
    results = queryTFS("FD_RMDIR " + std::string(dir));
    if (results[0] != "TM_ACK")
    {
        errno = std::stoi(results[0]);
        log("ERROR: _TFSrmdir_ failed, errno = " + results[0]);
        return -errno;
    }
    return 0;
}

/**
 * utime() filesystem operation implementation for TaggableFS.
 *
 * @param file relative path to file in the mounted filesystem.
 * @param ubuf buffer for utime() to store values in.
 * @return Return value from utime() called on the actual file.
 */
int TFSutime(const char *file, struct utimbuf *ubuf)
{
    log("_TFSutime_");
    std::string pathToFile = getRealPath(file, true);
    if (getFilename(pathToFile) == "")
    {
        log("ERROR: _TFSmknod_ getRealPath() failed");
        return -1;
    }
    int returnValue = utime(pathToFile.c_str(), ubuf);
    if (returnValue == -1)
    {
        log("ERROR: _TFSutime_ utime() failed, errno = " + std::to_string(errno));
        return -errno;
    }
    return returnValue;
}

}

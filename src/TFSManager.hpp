/**
 * @file TFSManager.hpp
 * @author Santhosh Ranganathan
 * @brief The header file for the TFSManager class.
 *
 * @details This file contains the class definition for the TFSManager class.
 * The TFSManager class handles all functionalities related to TaggableFS'
 * filesystem and answers the queries from FUSE operations and the command
 * line queries from the user. It stores and updates files in the root
 * directory named with their hash values and stores the actual filenames
 * in a database. Folders and tags are saved to the same table. The initial
 * idea was have a unified view where tagged files are in a reserved folder
 * and allow tagging by creating folders and moving files in a file manager.
 * In the final submitted version, there are instead two modes: one to copy
 * files into the filesystem and the other to peruse, create and delete tags.
 */

#ifndef TFS_TFSMANAGER_HPP
#define TFS_TFSMANAGER_HPP

#include "common.hpp"
#include "FUSEFileSystem.hpp"
#include <sqlite3.h>
#include <openssl/md5.h>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <set>

namespace TaggableFS
{

/**
 * This class handles queries from FUSE operations and command line queries from the user.
 */
class TFSManager
{
private:
    /** Path at which the FUSE filesystem is to be mounted. */
    std::string mountPoint;

    /** Folder where files saved to the filesystem are stored along with metadata. */
    std::string rootDirectory;

    /** Name of the original program to be passed to fuse_main(). */
    std::string programName;

    /** SQLite database to store and retrieve metadata regarding files, folders and tags. */
    sqlite3 *db;

    /** Path to database containing metadata inside the root directory. */
    std::string dbPath;

    /** Sending message queue descriptor for FUSE filesystem. */
    mqd_t txFUSEMQ;

    /** Sending message queue descriptor for QueryHandler. */
    mqd_t txQueryMQ;

    /** Receiving message queue descriptor. */
    mqd_t rxMQ;

    /** Buffer to store messages to be sent/received. */
    char buffer[TFS_MQ_MESSAGE_SIZE];

    /** File to store logs, saved inside root directory. */
    std::ofstream logFile;

    /** Check to see if logging is enabled. */
    bool enableLogging;

    /** Option to initialize FUSE filesystem in tag view mode. */
    bool tagView;

    void startDaemon();
    void initMQ();
    void loadDBFromStorage();
    void initDB();
    void prepareStatements();
    void finalizeStatements();
    void initFUSEFileSystem();
    void saveDBToStorage();
    void shutdown();
    void run();
    void messageFUSEFileSystem(std::string message, bool complete = true);
    void messageQueryHandler(std::string message, bool complete = true);
    bool dispatch(Message m);

    std::string calculateHash(std::string path);
    std::string dbExecuteSV(sqlite3_stmt *stmt); // execute and retrive single value
    std::vector<std::string> dbExecuteMV(sqlite3_stmt *stmt); // retrive multiple values
    std::vector<std::vector<std::string>> dbExecuteMR(sqlite3_stmt *stmt); // retrive multiple rows
    void log(std::string text);

    /**************************************************************************
     * Folder methods
     *************************************************************************/

    std::string getFileID(std::string filename, std::string parentFolderID);
    std::vector<std::string> getFileIDsInFolder(std::string parentFolderID);
    std::string getFilenameFromID(std::string fileID);
    std::string getFolderID(std::string folderName, std::string parentFolderID);
    std::string getFolderID(std::vector<std::string> &partsOfPath);
    std::string getHash(std::string filename, std::string parentFolderID);
    bool isFolderEmpty(std::string folderID);
    void updateHash(std::string fileID, std::string newHash);
    std::string getFilePath(std::string relativePath);
    std::vector<std::string> listFolder(std::string folderPath);
    int createFolder(std::string folderPath);
    int deleteFolder(std::string folderPath);
    int deleteFile(std::string filePath, std::vector<std::string> *savedTagIDs = NULL);
    int renamePath(std::string oldPath, std::string newPath);
    int truncateFile(off_t length, std::string filePath);
    void updateFile(std::string filePath);
    void addTemporaryFile(std::string tempFilePath);

    /**************************************************************************
     * Tag methods
     *************************************************************************/

    std::string getTagID(std::string tagPath);
    std::string getParentTagIDFromPath(std::string tagPath);
    std::string getTaggedFileID(std::string parentTagID, std::string filename);
    std::string getTagNameFromID(std::string tagID);
    std::vector<std::string> getAllTagIDs();
    std::vector<std::string> getParentTagIDs(std::string tagID);
    void getAncestorTagIDs(std::string tagID, std::set<std::string> &ancestors);
    std::vector<std::string> getChildTagIDs(std::string tagID);
    std::vector<std::string> getFileIDsUnderTagID(std::string tagID);
    std::vector<std::string> getFilenamesUnderTagID(std::string tagID);
    std::vector<std::string> listTagChildren(std::string tagID);
    std::string getTaggedFilePath(std::string relativePath);
    int createTag(std::string tagPath);
    int deleteTag(std::string tagPath);
    void updateTagFileIDs(std::string tagID, std::vector<std::string> fileIDs);
    void updateParentTagIDs(std::string tagID, std::vector<std::string> parentTagIDs);
    void updateChildTagIDs(std::string tagID, std::vector<std::string> parentTagIDs);
    int tagSingleFile(std::string fileID, std::string tagID);
    int untagSingleFile(std::string fileID, std::string tagID);
    int tagFiles(std::string filePath, std::string tag);
    int untagFiles(std::string filePath, std::string tag);
    int nestTag(std::string tagID, std::string parentTagID);
    int unnestTag(std::string tagID, std::string parentTagID);
    std::vector<std::string> getFileTags(std::string fileID);
    std::vector<std::string> findFileIDsWithTags(std::vector<std::string> tags);
    std::vector<std::string> findFileIDsWithAnyOfTags(std::vector<std::string> tags);
    int renameTaggedPath(std::string oldPath, std::string newPath);

public:
    TFSManager(std::string mountPoint, std::string rootDirectory,
        std::string programName, bool enableLogging, bool tagView);
    int init();
};

}

#endif
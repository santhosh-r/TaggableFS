/**
 * @file TFSManager.cpp
 * @author Santhosh Ranganathan
 * @brief The source file for the TFSManager class.
 *
 * @details This file contains the method definitions for the TFSManager class.
 */

#include "TFSManager.hpp"

/** Macro to simplify code to bind a string to the given sqlite statement. */
#define macro_bind_text(stmt, text) sqlite3_bind_text(stmt, \
    sqlite3_bind_parameter_index(stmt, (std::string("@") + #text).c_str()), \
    text.c_str(), text.size(), SQLITE_STATIC)

/** Macro to simplify code to bind a string containing an integer to the given sqlite statement. */
#define macro_bind_int(stmt, text) sqlite3_bind_int(stmt, \
    sqlite3_bind_parameter_index(stmt, (std::string("@") + #text).c_str()), \
    std::stoi(text))

namespace TaggableFS
{

/**
 * An enum to help with accessing the SQLite prepared statement objects.
 */
enum SQLitePreparedStatementObjects
{
    QH_STATS_1,
    QH_STATS_2,
    GET_FILE_ID,
    GET_FILE_IDS_IN_FOLDER,
    GET_FILENAME_FROM_ID,
    GET_FOLDER_ID,
    GET_HASH,
    IS_FOLDER_EMPTY,
    UPDATE_HASH,
    LIST_FOLDER_1,
    LIST_FOLDER_2,
    CREATE_FOLDER,
    DELETE_FOLDER,
    DELETE_FILE,
    RENAME_PATH_1,
    RENAME_PATH_2,
    ADD_TEMPORARY_FILE,
    GET_TAG_ID,
    GET_TAG_NAME_FROM_ID,
    GET_ALL_TAG_IDS,
    GET_PARENT_TAG_IDS,
    GET_CHILD_TAG_IDS,
    GET_FILE_IDS_UNDER_TAG_ID,
    GET_TAGGED_FILE_PATH,
    UPDATE_PARENT_TAG_IDS,
    UPDATE_CHILD_TAG_IDS,
    CREATE_TAG,
    DELETE_TAG,
    UPDATE_TAG_FILE_IDS,
    GET_FILE_TAGS,
    RENAME_TAGGED_PATH,
    COUNT_HASH_GT_0,
    COUNT_HASH_GT_1
};

/**
 * Constant to store the total number of SQLite prepared statement objects.
 */
const int NUMBER_OF_SQLITE_PSO = 33;

/**
 * Array of SQLite prepared statement objects to be used in the program to avoid possible SQL
 * injections.
 */
sqlite3_stmt *stmts[NUMBER_OF_SQLITE_PSO];

/**
 * Helper function to create the SQLite prepared statement objects to avoid possible SQL
 * injections.
 *
 * @return Boolean indicating success or failure to prepare SQLite statements.
 */
void TFSManager::prepareStatements()
{
    std::string sqlStatements[] {
        /* QH_STATS_1 */ "SELECT COUNT(*) FROM files;",
        /* QH_STATS_2 */ "SELECT COUNT(*) FROM tags WHERE parent_folder='0';",
        /* GET_FILE_ID */ "SELECT file_id FROM files WHERE filename=@filename AND "
            "parent_folder=@parentFolderID;",
        /* GET_FILE_IDS_IN_FOLDER */ "SELECT file_id FROM files WHERE "
            "parent_folder=@parentFolderID;",
        /* GET_FILENAME_FROM_ID */ "SELECT filename FROM files WHERE file_id=@fileID;",
        /* GET_FOLDER_ID */ "SELECT tag_id FROM tags WHERE tag_name=@folderName AND "
            "parent_folder=@parentFolderID;",
        /* GET_HASH */ "SELECT hash FROM files WHERE filename=@filename AND "
            "parent_folder=@parentFolderID;",
        /* IS_FOLDER_EMPTY */ "SELECT COUNT(*) > 0 FROM files WHERE parent_folder=@folderID;",
        /* UPDATE_HASH */ "UPDATE files SET hash=@newHash WHERE file_id=@fileID;",
        /* LIST_FOLDER_1 */ "SELECT tag_name FROM tags WHERE parent_folder=@folderID;",
        /* LIST_FOLDER_2 */ "SELECT filename FROM files WHERE parent_folder=@folderID;",
        /* CREATE_FOLDER */ "INSERT INTO tags ( tag_name, parent_folder ) VALUES "
            "( @newFolderName, @parentFolderID );",
        /* DELETE_FOLDER */ "DELETE FROM tags WHERE tag_id=@folderID;",
        /* DELETE_FILE */ "DELETE FROM files WHERE file_id=@fileID;",
        /* RENAME_PATH_1 */ "UPDATE files SET filename=@newName, parent_folder=@newParentFolderID "
            "WHERE file_id=@oldFileID;",
        /* RENAME_PATH_2 */ "UPDATE tags SET tag_name=@newName, parent_folder=@newParentFolderID "
            "WHERE tag_id=@oldFolderID;",
        /* ADD_TEMPORARY_FILE */ "INSERT INTO files ( filename, hash, parent_folder ) VALUES "
            "( @filename, @tempFilename, @parentFolderID );",
        /* GET_TAG_ID */ "SELECT tag_id FROM tags WHERE tag_name=@tag AND parent_folder='0';",
        /* GET_TAG_NAME_FROM_ID */ "SELECT tag_name FROM tags WHERE tag_id=@tagID;",
        /* GET_ALL_TAG_IDS */ "SELECT tag_id FROM tags WHERE parent_folder='0';",
        /* GET_PARENT_TAG_IDS */ "SELECT parent_tags FROM tags WHERE tag_id=@tagID;",
        /* GET_CHILD_TAG_IDS */ "SELECT child_tags FROM tags WHERE tag_id=@tagID;",
        /* GET_FILE_IDS_UNDER_TAG_ID */ "SELECT files_ids FROM tags WHERE tag_id=@tagID;",
        /* GET_TAGGED_FILE_PATH */ "SELECT hash FROM files WHERE file_id=@fileID;",
        /* UPDATE_PARENT_TAG_IDS */ "UPDATE tags SET parent_tags=@serializedIDs WHERE "
            "tag_id=@tagID;",
        /* UPDATE_CHILD_TAG_IDS */ "UPDATE tags SET child_tags=@serializedIDs WHERE "
            "tag_id=@tagID;",
        /* CREATE_TAG */ "INSERT INTO tags ( tag_name, parent_folder, parent_tags, child_tags, "
            "files_ids ) VALUES ( @tag, '0', @parentTags, '', '' );",
        /* DELETE_TAG */ "DELETE FROM tags WHERE tag_id=@tagID;",
        /* UPDATE_TAG_FILE_IDS */ "UPDATE tags SET files_ids=@serializedIDs WHERE tag_id=@tagID;",
        /* GET_FILE_TAGS */ "SELECT tag_id, tag_name, files_ids FROM tags WHERE parent_folder='0';",
        /* RENAME_TAGGED_PATH */ "UPDATE tags SET tag_name=@newName WHERE tag_id=@oldTagID;",
        /* COUNT_HASH_GT_0 */ "SELECT COUNT(*) > 0 FROM files WHERE hash=@oldhash;",
        /* COUNT_HASH_GT_1 */ "SELECT COUNT(*) > 1 FROM files WHERE hash=@hash;"
    };

    for (auto i = 0; i < NUMBER_OF_SQLITE_PSO; i++)
    {
        auto result = sqlite3_prepare_v2(db, sqlStatements[i].c_str(), -1, &stmts[i], NULL);
        if (result != SQLITE_OK)
        {
            log("TFSManager sqlite3_prepare_v2() failed, ERROR: " + std::string(sqlite3_errmsg(db)));
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * Finalize the SQLite prepared statement objects when done using them.
 */
void TFSManager::finalizeStatements()
{
    for (auto i = 0; i < NUMBER_OF_SQLITE_PSO; i++)
    {
        sqlite3_finalize(stmts[i]);
    }
}

/**
 * Constructor for the TFSManager class.
 *
 * @param mountPoint path at which FUSE filesystem is to be mounted.
 * @param rootDirectory root directory where files and metadata are stored.
 * @param programName name of the program creating the daemon.
 * @param enableLogging boolean to enable/disable logging.
 * @param tagView boolean to enable/disable tag view mode.
 */
TFSManager::TFSManager(std::string mountPoint, std::string rootDirectory,
                       std::string programName, bool enableLogging, bool tagView)
        : mountPoint(mountPoint), rootDirectory(rootDirectory), programName(programName),
          db(NULL), enableLogging(enableLogging), tagView(tagView)
{
}

/**
 * Creates required folder and logfile if non-existent and initializes TaggableFS daemon
 * which initializes other components.
 *
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::init()
{
    mkdir((rootDirectory + "/metadata").c_str(), 0755);
    if (enableLogging)
    {
        logFile = std::ofstream(rootDirectory + "/metadata/log.txt",
            std::ios::out | std::ios::app);
        if (logFile.fail())
        {
            perror("ERROR: Unable to create/open log file.");
            return errno;
        }
    }
    dbPath = rootDirectory + "/metadata/fs.db";

    int pid = fork();
    if (pid == -1)
    {
        perror("ERROR: TFSManager fork() failed");
        return errno;
    }
    if (pid == 0)
    {
        startDaemon();
    }
    return 0;
}

/**
 * Daemonizes TaggableFS Manager, initializes message queues, database storing metadata,
 * FUSE filesystem and begins dispatching queries from both FUSE operations and user and
 * exits when completed.
 */
void TFSManager::startDaemon()
{
    daemon(1, 0); // daemonize TaggableFS manager, doesn't double fork
    initMQ();
    initDB();
    initFUSEFileSystem();
    run();
    shutdown();
    exit(EXIT_SUCCESS);
}

/**
 * Prints given text to the log file.
 *
 * @param text text to be logged.
 */
void TFSManager::log(std::string text)
{
    if (enableLogging == true)
    {
        time_t t = time(NULL);
        logFile << std::put_time(localtime(&t), "%c") << " " << text << '\n';
        // logFile.flush(); // uncomment to avoid losing logs when debugging.
    }
}

/**
 * Initilaizes message queues to send/receive messages to and from FUSE operations and
 * QueryHandler.
 */
void TFSManager::initMQ()
{
    // initialize message queues
    mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = TFS_MQ_MAX_MESSAGES;
    attr.mq_msgsize = TFS_MQ_MESSAGE_SIZE;
    attr.mq_curmsgs = 0;

    mode_t existingMask = umask(0);
    txFUSEMQ = mq_open("/tfs_fusemq", O_WRONLY | O_CREAT | O_EXCL, 0660, &attr);
    txQueryMQ = mq_open("/tfs_querymq", O_WRONLY | O_CREAT | O_EXCL, 0660, &attr);
    rxMQ = mq_open("/tfs_managermq", O_RDONLY | O_CREAT | O_EXCL, 0660, &attr);
    umask(existingMask);
    if ( txFUSEMQ == -1 || txQueryMQ == -1 || rxMQ == -1 )
    {
        log("TFSManager mq_open() failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * Initializes FUSE filesystem after forking.
 */
void TFSManager::initFUSEFileSystem()
{
    int pid = fork();
    if (pid == 0)
    {
        FUSEFileSystem fuseDriver(mountPoint, programName, enableLogging);
        exit(EXIT_SUCCESS);
    }
}

/**
 * Executes SQLite prepared statement and returns single value. All required parameters 
 * need to be already bound.
 *
 * @param stmt SQLite prepared statement objected to be executed.
 * @return First column first row value resulting from execution of SQLite statement.
 */
std::string TFSManager::dbExecuteSV(sqlite3_stmt *stmt)
{
    std::string value = "";
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        value = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    }
    // log("TFSManager sqlite3_step() in dbExecuteSV() done, sqlite3_errmsg() -> " +
    //     std::string(sqlite3_errmsg(db)));
    sqlite3_reset(stmt);
    log(std::string("PSO ") + sqlite3_expanded_sql(stmt) + " -> " + value);
    return value;
}

/**
 * Executes SQLite statement and returns multiple values. All required parameters need to 
 * be already bound.
 *
 * @param stmt SQLite prepared statement objected to be executed.
 * @return First column values resulting from execution of SQLite statement.
 */
std::vector<std::string> TFSManager::dbExecuteMV(sqlite3_stmt *stmt)
{
    std::vector<std::string> values;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        values.push_back(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
    }    
    // log("TFSManager sqlite3_step() in dbExecuteMV() done, sqlite3_errmsg() -> " +
    //     std::string(sqlite3_errmsg(db)));
    sqlite3_reset(stmt);
    log(std::string("PSO ") + sqlite3_expanded_sql(stmt) + " -> 0:" +
        (values.empty() ? "" : values[0]));
    return values;
}

/**
 * Executes SQLite statement and returns values in multiple rows. All required parameters 
 * need to be already bound.
 *
 * @param stmt SQLite prepared statement objected to be executed.
 * @return All values in all rows resulting from execution of SQLite statement.
 */
std::vector<std::vector<std::string>> TFSManager::dbExecuteMR(sqlite3_stmt *stmt)
{
    std::vector<std::vector<std::string>> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::vector<std::string> rowValues;
        int numberOfColumns = sqlite3_column_count(stmt);
        for (auto i = 0; i < numberOfColumns; i++)
        {
            rowValues.push_back(reinterpret_cast<const char *>(sqlite3_column_text(stmt, i)));
        }
        rows.push_back(rowValues);
    }
    // log("TFSManager sqlite3_step() in dbExecuteMR() done, sqlite3_errmsg() -> " +
    //     std::string(sqlite3_errmsg(db)));
    sqlite3_reset(stmt);
    log(std::string("PSO ") + sqlite3_expanded_sql(stmt) + " -> 0:" +
        (rows.empty() ? "" : serializeStrings(rows[0], ',')));
    return rows;
}

/**
 * Loads database stored in file to in-memory database.
 * Based on example from https://www.sqlite.org/backup.html.
 * In-memory database speeds up access.
 */
void TFSManager::loadDBFromStorage()
{
    int result;
    sqlite3 *source;
    sqlite3_backup *backup;
    result = sqlite3_open(dbPath.c_str(), &source);
    if (result != SQLITE_OK)
    {
        log("TFSManager sqlite3_open() failed, ERROR: "
            + std::string(sqlite3_errmsg(source)));
        sqlite3_close(source);
        exit(EXIT_FAILURE);
    }
    backup = sqlite3_backup_init(db, "main", source, "main");
    if (backup != NULL)
    {
        sqlite3_backup_step(backup, -1);
        sqlite3_backup_finish(backup);
    }
    else // load failed
    {
        log("TFSManager sqlite3_backup_init() failed, ERROR: "
            + std::string(sqlite3_errmsg(db)));
        sqlite3_close(source);
        exit(EXIT_FAILURE);
    }
}

/**
 * Saves in-memory database to database file.
 * Based on example from https://www.sqlite.org/backup.html.
 * If TFSManager crashes, chanages to the filesystem are not saved.
 */
void TFSManager::saveDBToStorage()
{
    int result;
    sqlite3 *destination;
    sqlite3_backup *backup;
    result = sqlite3_open(dbPath.c_str(), &destination);
    if (result != SQLITE_OK)
    {
        log("TFSManager sqlite3_open() failed, ERROR: "
            + std::string(sqlite3_errmsg(destination)));
        sqlite3_close(destination);
        exit(EXIT_FAILURE);
    }
    backup = sqlite3_backup_init(destination, "main", db, "main");
    if (backup != NULL)
    {
        sqlite3_backup_step(backup, -1);
        sqlite3_backup_finish(backup);
    }
    else // save failed
    {
        log("TFSManager sqlite3_backup_init() failed, ERROR: "
            + std::string(sqlite3_errmsg(destination)));
        sqlite3_close(destination);
        exit(EXIT_FAILURE);
    }
}

/**
 * Initializes metadata database by creating database or loading from database file.
 */
void TFSManager::initDB()
{
    sqlite3_initialize();

    bool dbExists = (access(dbPath.c_str(), F_OK) == 0);
    if (sqlite3_open(":memory:", &db) != SQLITE_OK)
    {
        log("TFSManager sqlite3_open() failed, ERROR: " + std::string(sqlite3_errmsg(db)));
        exit(EXIT_FAILURE);
    }
    if (!dbExists) // create tables
    {
        const std::string statement = "CREATE TABLE tags ( tag_id INTEGER PRIMARY KEY NOT NULL, "
                "tag_name text NOT NULL, parent_folder INTEGER NOT NULL, "
                "parent_tags TEXT, child_tags TEXT, files_ids TEXT, "
                "FOREIGN KEY(parent_folder) REFERENCES tags(tag_id) );"
            "CREATE TABLE files ( file_id INTEGER PRIMARY KEY NOT NULL, "
                "filename TEXT NOT NULL, hash TEXT NOT NULL, parent_folder INTEGER, "
                "FOREIGN KEY(parent_folder) REFERENCES tags(tag_id) );"
            "INSERT INTO tags ( tag_id, tag_name, parent_folder, parent_tags, "
                "child_tags, files_ids ) VALUES ( 0, '__TaggableFS__//', '-1', '', '', '' );"
            "INSERT INTO tags ( tag_id, tag_name, parent_folder, parent_tags, "
                "child_tags, files_ids ) VALUES ( 1, '/', '-1', '', '', '' );";
        log(statement);
        int result = sqlite3_exec(db, statement.c_str(), NULL, NULL, NULL);
        if (result != SQLITE_OK)
        {
            log("TFSManager sqlite3_exec() failed, ERROR: " + std::string(sqlite3_errmsg(db)));
            exit(EXIT_FAILURE);
        }
        // insert initial values for variables
    }
    else // retreive values
    {
        loadDBFromStorage();
        // retrieve variables
    }
    prepareStatements(); // ready SQLite prepared statements
}

/**
 * Shuts down TaggableFS by unmounting FUSE filesystem, closing and deleting message queues,
 * saving database to file and closing the log file.
 */
void TFSManager::shutdown()
{
    fuse_unmount(mountPoint.c_str(), NULL);

    mq_close(txFUSEMQ);
    mq_unlink("/tfs_fusemq");
    mq_close(txQueryMQ);
    mq_unlink("/tfs_querymq");
    mq_close(rxMQ);
    mq_unlink("/tfs_managermq");

    // store variables
    finalizeStatements();
    saveDBToStorage();
    sqlite3_close(db);
    sqlite3_shutdown();

    logFile.close();
}

/**
 * Calculates hash of the file stored in path.
 *
 * @param path path to the file to be hashed.
 * @return Hash value as a string.
 */
std::string TFSManager::calculateHash(std::string path)
{
    unsigned char md5Value[MD5_DIGEST_LENGTH];
    unsigned char buf[4096];
    int fd = open(path.c_str(), O_RDONLY);
    int bytesRead = 0;

    MD5_CTX md5Context;
    MD5_Init(&md5Context);
    while ((bytesRead = read(fd, buf, 4096)) > 0)
    {
        MD5_Update(&md5Context, buf, bytesRead);
    }
    MD5_Final(md5Value, &md5Context);

    char md5String[2*MD5_DIGEST_LENGTH + 1];
    std::string hexSymbols = "0123456789ABCDEF";
    for (auto i = 0, j = 0; i < 2*MD5_DIGEST_LENGTH; i += 2, j++)
    {
        md5String[i] = hexSymbols[md5Value[j] >> 4];
        md5String[i+1] = hexSymbols[md5Value[j] & 0xF];
    }
    md5String[2*MD5_DIGEST_LENGTH] = '\0';
    return std::string(md5String);
}

/**
 * Runs TaggableFS until QUIT message is received from either FUSEFileSystem
 * after unmount or QueryHandler.
 */
void TFSManager::run()
{
    Message m;
    do // dispatch messages
    {
        mq_receive(rxMQ, buffer, TFS_MQ_MESSAGE_SIZE, NULL);
        m = deserializeMessage(buffer);
        log("MESSAGE: " + std::string(m.content));
    } while (dispatch(m));
}

/**
 * Sends message to FUSE operations.
 *
 * @param m message to be sent.
 * @param complete boolean indicating if message is complete or not.
 */
void TFSManager::messageFUSEFileSystem(std::string m, bool complete)
{
    serializeMessage(m.c_str(), buffer, complete);
    mq_send(txFUSEMQ, buffer, TFS_MQ_MESSAGE_SIZE, 0);
}

/**
 * Sends message to QueryHandler.
 *
 * @param m message to be sent.
 * @param complete boolean indicating if message is complete or not.
 */
void TFSManager::messageQueryHandler(std::string m, bool complete)
{
    serializeMessage(m.c_str(), buffer, complete);
    mq_send(txQueryMQ, buffer, TFS_MQ_MESSAGE_SIZE, 0);
}

/**
 * Dispatches query messages received from both FUSE operations and QueryHandler.
 *
 * @param m Message struct containing message to be dispatced.
 * @return Boolean indicating if QUIT message was received from FUSEFileSystem or QueryHandler.
 */
bool TFSManager::dispatch(Message m)
{
    static long loops = 0;
    loops++;
    std::vector<std::string> tokens = splitAtFirstOccurance(m.content);
    std::string query = tokens[0];
    if (query == "QH_TEST")
    {
        messageQueryHandler("TM_ACK (messages dispatched: " + std::to_string(loops) + ")");
    }
    else if (query == "QH_EXIT" || query == "FD_EXIT")
    {
        return false;
    }
    else if (query == "FD_TEST" || query == "FD_LOG")
    {
        messageFUSEFileSystem("TM_ACK");
    }
    else if (query == "FD_GET_PATH" || query == "FD_GET_PATH_WRITE")
    {
        std::string realPath = "";
        if (tagView == false)
        {
            realPath = getFilePath(tokens[1]);
        }
        else if (query != "FD_GET_PATH_WRITE")
        {
            realPath = getTaggedFilePath(tokens[1]);
        }
        messageFUSEFileSystem(realPath);
    }
    else if (query == "FD_IF_DIR")
    {
        bool isDirectory;
        if (tagView == true)
        {
            isDirectory = (getTagID(tokens[1]) != "");
        }
        else
        {
            std::vector<std::string> parts = splitPathIntoParts(tokens[1]);
            isDirectory = (getFolderID(parts) != "");
        }
        messageFUSEFileSystem(isDirectory ? "TM_TRUE" : "TM_FALSE");
    }
    else if (query == "FD_READ_DIR")
    {
        std::vector<std::string> contents;
        if (tagView == false)
        {
            contents = listFolder(tokens[1]);
        }
        else
        {
            contents = listTagChildren(tokens[1]);
        }
        std::size_t size = contents.size();
        if (size == 0)
        {
            messageFUSEFileSystem("");
        }
        else
        {
            for (std::size_t i = 0; i < size; i++)
            {
                bool complete = (i == size - 1);
                messageFUSEFileSystem(contents[i], complete);
            }
        }
    }
    else if (query == "FD_MKDIR")
    {
        int returnValue = 1;
        if (tagView == false)
        {
            returnValue = createFolder(tokens[1]);
        }
        else
        {
            returnValue = createTag(tokens[1]);
        }
        if (returnValue == 0)
        {
            messageFUSEFileSystem("TM_ACK");
        }
        else // folder/tag wasn't created
        {
            messageFUSEFileSystem(std::to_string(returnValue));
        }
    }
    else if (query == "FD_RMDIR")
    {
        int returnValue = 1;
        if (tagView == false)
        {
            returnValue = deleteFolder(tokens[1]);
        }
        else
        {
            returnValue = deleteTag(tokens[1]);
        }
        if (returnValue == 0)
        {
            messageFUSEFileSystem("TM_ACK");
        }
        else // folder/tag doesn't exist or wasn't deleted
        {
            messageFUSEFileSystem(std::to_string(returnValue));
        }
    }
    else if (query == "FD_UNLINK")
    {
        int returnValue = 1;
        if (tagView == false)
        {
            returnValue = deleteFile(tokens[1]);
        }
        else // untag file, not delete
        {
            std::string tagID = getParentTagIDFromPath(tokens[1]);
            std::string fileID = getTaggedFileID(tagID, getFilename(tokens[1]));
            returnValue = untagSingleFile(fileID, tagID);
        }
        if (returnValue == 0)
        {
            messageFUSEFileSystem("TM_ACK");
        }
        else // file doesn't exist or wasn't deleted
        {
            messageFUSEFileSystem(std::to_string(returnValue));
        }
    }
    else if (query == "FD_RENAME")
    {
        int returnValue = 1;
        std::vector<std::string> arguments = splitAtFirstOccurance(tokens[1], ',');
        if (tagView == false)
        {
            returnValue = renamePath(arguments[0], arguments[1]);
        }
        else
        {
            returnValue = renameTaggedPath(arguments[0], arguments[1]);
        }
        // non-zero return value -> file doesn't exist or wasn't renamed
        messageFUSEFileSystem((returnValue == 0) ? "TM_ACK" : "TM_FAIL");
    }
    else if (query == "FD_TRUNCATE")
    {
        int returnValue = 1;
        std::vector<std::string> arguments = splitAtFirstOccurance(tokens[1], ',');
        if (tagView == false) // tag view mode read only
        {
            off_t length = std::stol(arguments[0]);
            std::string filePath = arguments[1];
            returnValue = truncateFile(length, filePath);
        }
        if (returnValue == 0)
        {
            messageFUSEFileSystem("TM_ACK");
        }
        else // file doesn't exist or wasn't truncated
        {
            messageFUSEFileSystem(std::to_string(returnValue));
        }
    }
    else if (query == "FD_UPDATE")
    {
        if (tagView == false) // tag view mode read only
        {
            updateFile(tokens[1]);
        }
        messageFUSEFileSystem("TM_ACK");
    }
    else if (query == "FD_ADD_TEMP") // in tag view mode, mknod will fail before sending this
    {
        addTemporaryFile(tokens[1]);
        messageFUSEFileSystem("TM_ACK");
    }
    else if (query == "QH_TAG")
    {
        std::vector<std::string> arguments = splitAtFirstOccurance(tokens[1], ',');
        int returnValue = tagFiles(arguments[0], arguments[1]);
        if (returnValue == 0)
        {
            messageQueryHandler("File(s) successfully tagged.");
        }
        else if (returnValue == EEXIST)
        {
            messageQueryHandler("Failed. Filename conflict with "
                "files already tagged with the same tag.");
        }
        else
        {
            messageQueryHandler("Failed. Either file(s) path or tag is invalid.");
        }
    }
    else if (query == "QH_UNTAG")
    {
        std::vector<std::string> arguments = splitAtFirstOccurance(tokens[1], ',');
        int returnValue = untagFiles(arguments[0], arguments[1]);
        if (returnValue == 0)
        {
            messageQueryHandler("File(s) successfully untagged.");
        }
        else
        {
            messageQueryHandler("Failed. Either file(s) path or tag is invalid.");
        }
    }
    else if (query == "QH_NEST")
    {
        std::vector<std::string> arguments = splitAtFirstOccurance(tokens[1], ',');
        int returnValue = nestTag(getTagID(arguments[0]), getTagID(arguments[1]));
        if (returnValue == 0)
        {
            messageQueryHandler("Tag successfully nested.");
        }
        else if (returnValue == 1)
        {
            messageQueryHandler("Cyclic check error.");
        }
        else
        {
            messageQueryHandler("Failed. Either tag is invalid.");
        }
    }
    else if (query == "QH_UNNEST")
    {
        std::vector<std::string> arguments = splitAtFirstOccurance(tokens[1], ',');
        int returnValue = unnestTag(getTagID(arguments[0]), getTagID(arguments[1]));
        if (returnValue == 0)
        {
            messageQueryHandler("Tag successfully unnested.");
        }
        else
        {
            messageQueryHandler("Failed. Either tag is invalid.");
        }
    }
    else if (query == "QH_STATS")
    {
        int numberOfFiles = std::stoi(dbExecuteSV(stmts[QH_STATS_1]));
        int numberOfTags = std::stoi(dbExecuteSV(stmts[QH_STATS_2]));
        std::string stats = "Files: " + std::to_string(numberOfFiles)
            + ", Tags: " + std::to_string(numberOfTags);
        messageQueryHandler(stats);
    }
    else if (query == "QH_SEARCH")
    {
        std::vector<std::string> arguments = splitAtFirstOccurance(tokens[1], ',');
        std::vector<std::string> tags = deserializeStrings(arguments[1]);
        std::vector<std::string> ids;
        if (arguments[0] == "1")
        {
            ids = findFileIDsWithTags(tags);
        }
        else
        {
            ids = findFileIDsWithAnyOfTags(tags);
        }
        std::size_t size = ids.size();
        if (size == 0)
        {
            messageQueryHandler("");
        }
        else
        {
            for (std::size_t i = 0; i < size; i++)
            {
                bool complete = (i == size - 1);
                messageQueryHandler(getFilenameFromID(ids[i]), complete);
            }
        }
    }
    else if (query == "QH_CREATE_TAG")
    {
        int returnValue = createTag(tokens[1]);
        if (returnValue == 0)
        {
            messageQueryHandler("Tag successfully created.");
        }
        else if (returnValue == EEXIST)
        {
            messageQueryHandler("Failed. Tag already exists.");
        }
        else
        {
            messageQueryHandler("Failed. Given tag is invalid.");
        }
    }
    else if (query == "QH_DELETE_TAG")
    {
        int returnValue = deleteTag(tokens[1]);
        if (returnValue == 0)
        {
            messageQueryHandler("Tag successfully deleted.");
        }
        else if (returnValue == ENOTEMPTY)
        {
            messageQueryHandler("Failed. Tag is not empty and has tags and/or "
                "files nested under it.");
        }
        else
        {
            messageQueryHandler("Failed. Given tag is invalid.");
        }
    }
    else if (query == "QH_GET_TAGS")
    {
        std::vector<std::string> parts = splitPathIntoParts(tokens[1]);
        std::string filename = popBackAndRemove(parts);
        std::string folderID = getFolderID(parts);
        if (folderID == "")
        {
            messageQueryHandler("Invalid");
        }
        else
        {
            std::string fileID = getFileID(filename, folderID);
            if (fileID == "")
            {
                messageQueryHandler("Invalid");
            }
            else
            {
                std::vector<std::string> tags = getFileTags(fileID);
                messageQueryHandler(serializeStrings(tags));
            }
        }
    }
    return true;
}

/**************************************************************************************************
 * Folder methods
 *************************************************************************************************/

/**
 * Gets file ID from filename and parent folder's tag ID.
 *
 * @param filename filename whose file ID is to be found.
 * @param parentFolderID parent folder's tag ID.
 * @return File ID from the database as a string.
 */
std::string TFSManager::getFileID(std::string filename, std::string parentFolderID)
{
    macro_bind_text(stmts[GET_FILE_ID], filename);
    macro_bind_int(stmts[GET_FILE_ID], parentFolderID);
    return dbExecuteSV(stmts[GET_FILE_ID]);
}

/**
 * Gets the file IDs for all files inside a folder.
 *
 * @param parentFolderID parent folder's tag ID.
 * @return String vector containing file IDs inside the parent folder.
 */
std::vector<std::string> TFSManager::getFileIDsInFolder(std::string parentFolderID)
{
    macro_bind_int(stmts[GET_FILE_IDS_IN_FOLDER], parentFolderID);
    return dbExecuteMV(stmts[GET_FILE_IDS_IN_FOLDER]);
}

/**
 * Gets filename associated with the given file ID.
 *
 * @param fileID file ID whose filename is to be found.
 * @return Filename as a string.
 */
std::string TFSManager::getFilenameFromID(std::string fileID)
{
    macro_bind_int(stmts[GET_FILENAME_FROM_ID], fileID);
    return dbExecuteSV(stmts[GET_FILENAME_FROM_ID]);
}

/**
 * Gets folder's tag ID from folder's name and the parent folder's tag ID.
 *
 * @param folderName folder name whose tag ID is to be found.
 * @param parentFolderID parent folder's tag ID.
 * @return Folder's tag ID as a string.
 */
std::string TFSManager::getFolderID(std::string folderName, std::string parentFolderID)
{
    macro_bind_text(stmts[GET_FOLDER_ID], folderName);
    macro_bind_int(stmts[GET_FOLDER_ID], parentFolderID);
    return dbExecuteSV(stmts[GET_FOLDER_ID]);
}

/**
 * Gets folder's tag ID from the path to the folder.
 *
 * @param partsOfPath vector containing parts of the path to the folder.
 * @return Folder's tag ID as a string.
 */
std::string TFSManager::getFolderID(std::vector<std::string> &partsOfPath)
{
    std::string folderID;
    int size = partsOfPath.size();
    folderID = "1";
    for (auto i = 0; i < size; i++)
    {
        // sql query failed before exhausting path i.e. non-existent path
        if (folderID == "")
        {
            return "";
        }
        folderID = getFolderID(partsOfPath[i], folderID);
    }
    return folderID;
}

/**
 * Gets file's hash value from filename and the parent folder's tag ID.
 *
 * @param filename filename of file whose hash value is to be found.
 * @param parentFolderID parent folder's tag ID.
 * @return Hash value of the file from the database as a string.
 */
std::string TFSManager::getHash(std::string filename, std::string parentFolderID)
{
    macro_bind_text(stmts[GET_HASH], filename);
    macro_bind_int(stmts[GET_HASH], parentFolderID);
    return dbExecuteSV(stmts[GET_HASH]);
}

/**
 * Checks if a folder is empty.
 *
 * @param folderID tag ID of the folder to be checked.
 * @return Boolean indicating if folder is empty or not.
 */
bool TFSManager::isFolderEmpty(std::string folderID)
{
    macro_bind_int(stmts[IS_FOLDER_EMPTY], folderID);
    int isFolderEmpty = std::stoi(dbExecuteSV(stmts[IS_FOLDER_EMPTY]));
    return (isFolderEmpty == 0);
}

/**
 * Updates the hash value after a write or a truncate operation with the given hash value.
 *
 * @param fileID file ID of the file whose hash value is to be updated.
 * @param newHash new hash value to be inserted in place of the old value.
 */
void TFSManager::updateHash(std::string fileID, std::string newHash)
{
    macro_bind_text(stmts[UPDATE_HASH], newHash);
    macro_bind_int(stmts[UPDATE_HASH], fileID);
    dbExecuteSV(stmts[UPDATE_HASH]);
}

/**
 * Gets the actual path to the file inside the root directory from the mounted path.
 *
 * @param relativePath mounted path to file whose actual path is to be found.
 * @return Actual path to the file specified by the mounted path.
 */
std::string TFSManager::getFilePath(std::string relativePath)
{
    std::vector<std::string> parts = splitPathIntoParts(relativePath);
    std::string filename = popBackAndRemove(parts);
    std::string parentFolderID = getFolderID(parts);
    if (parentFolderID != "") // parent folder exists
    {
        // need not check if file exists, path will be used by mknod
        return rootDirectory + "/" + getHash(filename, parentFolderID);
    }
    return "";
}

/**
 * Lists contents of the folder specified by the given path.
 *
 * @param folderPath path to the folder whose contents are to be listed.
 * @return String vector containing contents of the folder.
 */
std::vector<std::string> TFSManager::listFolder(std::string folderPath)
{
    std::vector<std::string> parts = splitPathIntoParts(folderPath);
    std::string folderID = getFolderID(parts);
    std::vector<std::string> contents;
    if (folderID != "") // folder exists
    {
        macro_bind_int(stmts[LIST_FOLDER_1], folderID);
        contents = dbExecuteMV(stmts[LIST_FOLDER_1]);
        macro_bind_int(stmts[LIST_FOLDER_2], folderID);
        std::vector<std::string> filenames = dbExecuteMV(stmts[LIST_FOLDER_2]);
        contents.reserve(contents.size() + filenames.size());
        contents.insert(contents.end(), filenames.begin(), filenames.end());
    }
    return contents;
}

/**
 * Creates folder specified by given path by inserting a new row into tags table.
 *
 * @param folderPath path specifying where folder is to be created and its name.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::createFolder(std::string folderPath)
{
    std::vector<std::string> parts = splitPathIntoParts(folderPath);
    std::string newFolderName = popBackAndRemove(parts);
    std::string parentFolderID = getFolderID(parts);
    if (parentFolderID != "") // parent folder exists
    {
        std::string fileID = getFileID(newFolderName, parentFolderID);
        std::string folderID = getFolderID(newFolderName, parentFolderID);
        if (fileID == "" && folderID == "") // no conflict
        {
            macro_bind_text(stmts[CREATE_FOLDER], newFolderName);
            macro_bind_int(stmts[CREATE_FOLDER], parentFolderID);
            dbExecuteSV(stmts[CREATE_FOLDER]);
            return 0;
        }
        return EEXIST; // file/folder with same name exists
    }
    return ENOENT; // invalid path to folder to be created
}

/**
 * Deletes folder specified by given path by removing its row entry from tags table.
 *
 * @param folderPath path specifying folder to be deleted.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::deleteFolder(std::string folderPath)
{
    std::vector<std::string> parts = splitPathIntoParts(folderPath);
    std::string folderID = getFolderID(parts);
    if (folderID != "") // folder exists
    {
        if (isFolderEmpty(folderID) == true) // folder empty, proceed with delete
        {
            macro_bind_int(stmts[DELETE_FOLDER], folderID);
            dbExecuteSV(stmts[DELETE_FOLDER]);
            return 0;
        }
        return ENOTEMPTY; // folder not empty
    }
    return ENOENT; // path invalid
}

/**
 * Deletes file specified by given path by removing its row entry from the files table 
 * and deleting the actual file if there are no futher references to it.
 *
 * @param filePath path specifying file to be deleted.
 * @param savedTagIDs save tag IDs for file replacement.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::deleteFile(std::string filePath, std::vector<std::string> *savedTagIDs)
{
    int returnValue = 1;
    std::vector<std::string> parts = splitPathIntoParts(filePath);
    std::string filename = popBackAndRemove(parts);
    std::string parentFolderID = getFolderID(parts);
    if (parentFolderID != "") // parent folder exists
    {
        std::string hash = getHash(filename, parentFolderID);
        if (hash != "") // file exists
        {
            returnValue = 0;
            std::string filePath = rootDirectory + "/" + hash;
            macro_bind_text(stmts[COUNT_HASH_GT_1], hash);
            bool isLastFileWithHash = std::stoi(dbExecuteSV(stmts[COUNT_HASH_GT_1])) == 0;
            if (isLastFileWithHash) // delete actual file if its the last reference
            {
                returnValue = unlink(filePath.c_str());
                if (returnValue == -1)
                {
                    returnValue = errno;
                }
            }
            if (returnValue == 0) // either unlink successful or not last reference
            {
                // remove all references to file in tags
                std::string fileID = getFileID(filename, parentFolderID);
                std::vector<std::string> allTagIDs = getAllTagIDs();
                if (savedTagIDs != NULL)
                {
                    savedTagIDs->clear();
                }
                for (auto tagID : allTagIDs)
                {
                    std::vector<std::string> fileIDs = getFileIDsUnderTagID(tagID);
                    auto result = std::find(fileIDs.begin(), fileIDs.end(), fileID);
                    if (result != fileIDs.end())
                    {
                        fileIDs.erase(result);
                        updateTagFileIDs(tagID, fileIDs);
                        if (savedTagIDs != NULL)
                        {
                            savedTagIDs->push_back(tagID);
                        }
                    }
                }
                macro_bind_int(stmts[DELETE_FILE], fileID);
                dbExecuteSV(stmts[DELETE_FILE]);
            }
        }
    }
    return returnValue;
}

/**
 * Moves file or folder from old path to new path.
 *
 * @param oldPath path specifying file/folder to be moved.
 * @param newPath destination where file/folder is to be moved.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::renamePath(std::string oldPath, std::string newPath)
{
    std::vector<std::string> parts = splitPathIntoParts(oldPath);
    std::string oldName = popBackAndRemove(parts);
    std::string oldParentFolderID = getFolderID(parts);
    std::string oldFolderID = "", oldFileID = "";
    if (oldParentFolderID != "") // source parent folder exists
    {
        oldFolderID = getFolderID(oldName, oldParentFolderID);
        oldFileID = getFileID(oldName, oldParentFolderID);
    }
    parts = splitPathIntoParts(std::string(newPath));
    std::string newName = popBackAndRemove(parts);
    std::string newParentFolderID = getFolderID(parts);
    std::string newFolderID = "", newFileID = "";
    if (newParentFolderID != "") // destination parent folder exists
    {
        newFolderID = getFolderID(newName, newParentFolderID);
        newFileID = getFileID(newName, newParentFolderID);
    }
    if (oldFileID != "" && newFolderID == "")
    {
        std::vector<std::string> fileTags = getFileTags(oldFileID);
        for (auto tag : fileTags)
        {
            std::vector<std::string> filenames = getFilenamesUnderTagID(getTagID(tag));
            auto result = std::find(filenames.begin(), filenames.end(), newName);
            if (result != filenames.end())
            {
                return EEXIST;
            }
        }
        // old file exists, replace if new file exists
        std::vector<std::string> savedTagIDs;
        if (newFileID != "")
        {
            // TODO test if temporary system file beginning with ".goutput" etc.
            deleteFile(newPath, &savedTagIDs);
        }
        macro_bind_text(stmts[RENAME_PATH_1], newName);
        macro_bind_int(stmts[RENAME_PATH_1], newParentFolderID);
        macro_bind_int(stmts[RENAME_PATH_1], oldFileID);
        dbExecuteSV(stmts[RENAME_PATH_1]);
        for (auto tagID : savedTagIDs)
        {
            std::vector<std::string> fileIDs = getFileIDsUnderTagID(tagID);
            fileIDs.push_back(oldFileID);
            updateTagFileIDs(tagID, fileIDs);
        }
        return 0;
    }
    else if (oldFolderID != "" && newFolderID == "" && newFileID == "")
    {
        // folder exists, no replacement
        macro_bind_text(stmts[RENAME_PATH_2], newName);
        macro_bind_int(stmts[RENAME_PATH_2], newParentFolderID);
        macro_bind_int(stmts[RENAME_PATH_2], oldFolderID);
        dbExecuteSV(stmts[RENAME_PATH_2]);
        return 0;
    }
    return 1;
}

/**
 * Truncates specified file or a copy if there are other identical files.
 *
 * @param length length to which file is to be truncated.
 * @param filePath path specifying file to be truncated.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::truncateFile(off_t length, std::string filePath)
{
    int returnValue = 1;
    std::vector<std::string> parts = splitPathIntoParts(filePath);
    std::string filename = popBackAndRemove(parts);
    std::string parentFolderID = getFolderID(parts);
    if (parentFolderID != "") // parent folder exists
    {
        std::string hash = getHash(filename, parentFolderID);
        if (hash != "")
        {
            std::string filePath = rootDirectory + "/" + hash;
            bool copyMade = false;
            macro_bind_text(stmts[COUNT_HASH_GT_1], hash);
            if (std::stoi(dbExecuteSV(stmts[COUNT_HASH_GT_1])) == 1)
            {
                // truncate copy of file as other files with same hash exist
                std::string command = "cp " + filePath + " " + filePath + ".TRUNCATE";
                system(command.c_str());
                filePath += ".TRUNCATE";
                copyMade = true;
            }
            returnValue = truncate(filePath.c_str(), length);
            if (returnValue == 0) // update if needed after truncate
            {
                std::string newHash = calculateHash(filePath);
                // avoid renaming temp files
                if (newHash != hash && newHash != "D41D8CD98F00B204E9800998ECF8427E")
                {
                    rename(filePath.c_str(), (rootDirectory + "/" + newHash).c_str());
                    std::string fileID = getFileID(filename, parentFolderID);
                    updateHash(fileID, newHash);
                }
            }
            else
            {
                returnValue = errno;
            }
            if (copyMade == true)
            {
                remove(filePath.c_str());
            }
        }
        else
        {
            returnValue = ENOENT;
        }
    }
    return returnValue;
}

/**
 * Updates file's value specified by given path after a write or a truncate operation.
 *
 * @param filePath path specifying file whose hash value is to be updated.
 */
void TFSManager::updateFile(std::string filePath)
{
    std::vector<std::string> parts = splitPathIntoParts(filePath);
    std::string filename = popBackAndRemove(parts);
    std::string parentFolderID = getFolderID(parts); // assume path valid, file already open
    std::string oldHash = getHash(filename, parentFolderID);
    std::string tempFilePath = rootDirectory + "/" + oldHash + ".WRITE";
    bool tempExists = (access(tempFilePath.c_str(), F_OK) == 0);
    if (tempExists)
    {
        std::string newHash = calculateHash(tempFilePath);
        // avoid renaming temp files
        if (oldHash != newHash && newHash != "D41D8CD98F00B204E9800998ECF8427E")
        {
            rename(tempFilePath.c_str(), (rootDirectory + "/" + newHash).c_str());
            std::string fileID = getFileID(filename, parentFolderID);
            updateHash(fileID, newHash);
            macro_bind_text(stmts[COUNT_HASH_GT_0], oldHash);
            if (std::stoi(dbExecuteSV(stmts[COUNT_HASH_GT_0])) == 0)
            {
                remove((rootDirectory + "/" + oldHash).c_str());
            }
        }
        else
        {
            remove(tempFilePath.c_str());
        }
    }
}

/**
 * Adds new entry for newly created file to files table pointing to an empty temporary file.
 *
 * @param tempFilePath path to file to be created.
 */
void TFSManager::addTemporaryFile(std::string tempFilePath)
{
    std::vector<std::string> args = splitAtFirstOccurance(tempFilePath, ',');
    std::string tempFilename = args[0];
    std::vector<std::string> parts = splitPathIntoParts(args[1]);
    std::string filename = popBackAndRemove(parts);
    std::string parentFolderID = getFolderID(parts); // assume path vaild, path checked before
    macro_bind_text(stmts[ADD_TEMPORARY_FILE], filename);
    macro_bind_text(stmts[ADD_TEMPORARY_FILE], tempFilename);
    macro_bind_int(stmts[ADD_TEMPORARY_FILE], parentFolderID);
    dbExecuteSV(stmts[ADD_TEMPORARY_FILE]);
}

/**************************************************************************************************
 * Tag methods
 *************************************************************************************************/

/**
 * Formats IDs into comma separated values for use in SQLite query.
 *
 * @param ids vector of IDs to be formatted into csv string
 * @return String containing comma separated values.
 */
std::string formatIDsForSQL(std::vector<std::string> ids)
{
    std::string lastID = popBackAndRemove(ids);
    std::string idsFormattedForSQL = "";
    for (auto id : ids)
    {
        idsFormattedForSQL += "'" + id + "', ";
    }
    if (lastID != "")
    {
        idsFormattedForSQL += "'" + lastID + "'";
    }
    return idsFormattedForSQL;
}

/**
 * Gets tag ID from the mounted path to the tag in tag view mode.
 *
 * @param tagPath mounted path to the tag in tag view mode.
 * @return Tag ID as a string.
 */
std::string TFSManager::getTagID(std::string tagPath)
{
    if (tagPath == "")
    {
        return "";
    }
    if (tagPath == "/")
    {
        return "0";
    }
    std::string tag = tagPath; // if method called with tag only
    if (tagPath[0] == '/')
    {
        std::vector<std::string> parts = splitPathIntoParts(tagPath);
        std::string tagID = getTagID(popBackAndRemove(parts));
        std::set<std::string> ancestorIDs;
        getAncestorTagIDs(tagID, ancestorIDs);
        for (auto part : parts)
        {
            auto setResult = ancestorIDs.find(getTagID(part));
            if (setResult == ancestorIDs.end())
            {
                return "";
            }
        }
        return tagID;
    }
    macro_bind_text(stmts[GET_TAG_ID], tag);
    return dbExecuteSV(stmts[GET_TAG_ID]);
}

/**
 * Gets tag name from corresponding tag ID.
 *
 * @param tagID tag ID whose name is to be found.
 * @return tag name.
 */
std::string TFSManager::getTagNameFromID(std::string tagID)
{
    macro_bind_int(stmts[GET_TAG_NAME_FROM_ID], tagID);
    return dbExecuteSV(stmts[GET_TAG_NAME_FROM_ID]);
}

/**
 * Gets parent tag ID of a tag or tagged file specified by path in tag view mode.
 *
 * @param tagPath path to tag or tagged file in tag view mode.
 * @return Tag ID of the parent tag.
 */
std::string TFSManager::getParentTagIDFromPath(std::string tagPath)
{
    std::string parentTag = tagPath.substr(0, tagPath.find_last_of('/'));
    return (parentTag == "") ? "0" : getTagID(parentTag);
}

/**
 * Gets tagged file's file ID from its parent tag's tag ID and its filename.
 *
 * @param parentTagID tag ID of the parent tag.
 * @param filename filename of file whose file ID is to be found.
 * @return File ID of the tagged file specified.
 */
std::string TFSManager::getTaggedFileID(std::string parentTagID, std::string filename)
{
    if (parentTagID == "")
    {
        return "";
    }
    std::vector<std::string> fileIDs = getFileIDsUnderTagID(parentTagID);
    for (auto fileID : fileIDs)
    {
        if (filename == getFilenameFromID(fileID))
        {
            return fileID;
        }
    }
    return "";
}

/**
 * Gets tag IDs of all tags that exist in database.
 *
 * @return String vector containing tag IDs of all tags.
 */
std::vector<std::string> TFSManager::getAllTagIDs()
{
    return dbExecuteMV(stmts[GET_ALL_TAG_IDS]);
}

/**
 * Gets parent tag IDs under which given tag is nested under.
 *
 * @param tagID tagID whose parent tag IDs are to be found.
 * @return String vector containing tag IDs of specified tag's parent tags
 */
std::vector<std::string> TFSManager::getParentTagIDs(std::string tagID)
{
    macro_bind_int(stmts[GET_PARENT_TAG_IDS], tagID);
    return deserializeStrings(dbExecuteSV(stmts[GET_PARENT_TAG_IDS]));
}

/**
 * Gets tag IDs of all unique ancestors of a tag using recursion.
 *
 * @param tagID tagID whose ancestors' tag IDs are to be found.
 * @param ancestors set where ancestor's tag IDs are to be stored.
 */
void TFSManager::getAncestorTagIDs(std::string tagID, std::set<std::string> &ancestors)
{
    if (tagID == "" || tagID == "0") // Reserved id for root tag, can't be used as argument
    {
        return;
    }
    ancestors.insert(tagID);
    std::vector<std::string> parentIDs = getParentTagIDs(tagID);
    for (auto id : parentIDs)
    {
        getAncestorTagIDs(id, ancestors);
    }
}

/**
 * Gets tag IDs of child tags which are nested under the given tag.
 *
 * @param tagID tag ID of tag whose children are to be found.
 * @return String vector containing tag IDs of child tags nested under given tag.
 */
std::vector<std::string> TFSManager::getChildTagIDs(std::string tagID)
{
    macro_bind_int(stmts[GET_CHILD_TAG_IDS], tagID);
    return deserializeStrings(dbExecuteSV(stmts[GET_CHILD_TAG_IDS]));
}

/**
 * Gets file IDs which are tagged with tag specified by the given tag ID.
 *
 * @param tagID tag ID of tag with which files to be found are tagged with.
 * @return String vector containing file IDs of files tagged with the given tag.
 */
std::vector<std::string> TFSManager::getFileIDsUnderTagID(std::string tagID)
{
    if (tagID == "")
    {
        std::vector<std::string> empty;
        return empty;
    }
    macro_bind_int(stmts[GET_FILE_IDS_UNDER_TAG_ID], tagID);
    return deserializeStrings(dbExecuteSV(stmts[GET_FILE_IDS_UNDER_TAG_ID]));
}

/**
 * Gets filenames of files tagged with tag specified by the given tag ID.
 *
 * @param tagID tag ID of tag with which files to be found are tagged with.
 * @return String vector containing filenames of files tagged with the given tag.
 */
std::vector<std::string> TFSManager::getFilenamesUnderTagID(std::string tagID)
{
    std::vector<std::string> fileIDs = getFileIDsUnderTagID(tagID);
    if (fileIDs.empty())
    {
        return fileIDs;
    }
    std::vector<std::string> filenames;
    for (auto fileID : fileIDs)
    {
        macro_bind_int(stmts[GET_FILENAME_FROM_ID], fileID);
        filenames.push_back(dbExecuteSV(stmts[GET_FILENAME_FROM_ID]));
    }
    return filenames;
}

/**
 * Gets actual file path of the tagged file specified by its path in tag view mode.
 *
 * @param relativePath path to file in tag view mode.
 * @return Actual path to file specified by path.
 */
std::string TFSManager::getTaggedFilePath(std::string relativePath)
{
    std::string tagID = getParentTagIDFromPath(relativePath);
    std::string filename = getFilename(relativePath);
    if (tagID != "")
    {
        std::vector<std::string> fileIDs = getFileIDsUnderTagID(tagID);
        for (auto fileID : fileIDs)
        {
            if (filename == getFilenameFromID(fileID))
            {
                macro_bind_int(stmts[GET_TAGGED_FILE_PATH], fileID);
                return rootDirectory + "/" + dbExecuteSV(stmts[GET_TAGGED_FILE_PATH]);
            }
        }
    }
    return "";
}

/**
 * Lists files tagged with and tags nested under tag specified by given path in tag view mode.
 *
 * @param tagPath path to tag in tag view mode or tag name.
 * @return String vector containing files and tags under specified tag.
 */
std::vector<std::string> TFSManager::listTagChildren(std::string tagPath)
{
    std::string tagID = getTagID(tagPath);
    std::vector<std::string> contents;
    if (tagID != "")
    {
        std::vector<std::string> childTagIDs = getChildTagIDs(tagID);
        for (auto tagID : childTagIDs)
        {
            macro_bind_int(stmts[GET_TAG_NAME_FROM_ID], tagID);
            contents.push_back(dbExecuteSV(stmts[GET_TAG_NAME_FROM_ID]));
        }
        std::vector<std::string> filenames = getFilenamesUnderTagID(tagID);
        contents.reserve(contents.size() + filenames.size());
        contents.insert(contents.end(), filenames.begin(), filenames.end());
    }
    return contents;
}

/**
 * Updates parent tag IDs of tag specified by tag ID after un/nesting operation.
 *
 * @param tagID tag ID of tag whose parent tag IDs are to be updated.
 * @param parentTagIDs vector containing tag's parent tag IDs.
 */
void TFSManager::updateParentTagIDs(std::string tagID, std::vector<std::string> parentTagIDs)
{
    std::string serializedIDs = serializeStrings(parentTagIDs);
    macro_bind_text(stmts[UPDATE_PARENT_TAG_IDS], serializedIDs);
    macro_bind_int(stmts[UPDATE_PARENT_TAG_IDS], tagID);
    dbExecuteSV(stmts[UPDATE_PARENT_TAG_IDS]);
}

/**
 * Updates child tag IDs of tag specified by tag ID after un/nesting operation.
 *
 * @param tagID tag ID of tag whose child tag IDs are to be updated.
 * @param childTagIDs vector containing tag's child tag IDs.
 */
void TFSManager::updateChildTagIDs(std::string tagID, std::vector<std::string> childTagIDs)
{
    std::string serializedIDs = serializeStrings(childTagIDs);
    macro_bind_text(stmts[UPDATE_CHILD_TAG_IDS], serializedIDs);
    macro_bind_int(stmts[UPDATE_CHILD_TAG_IDS], tagID);
    dbExecuteSV(stmts[UPDATE_CHILD_TAG_IDS]);
}

/**
 * Creates tag using given tag name or path containing tag name in tag view mode.
 *
 * @param tagPath new tag's name or path in tag view mode containing it.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::createTag(std::string tagPath)
{
    if (getTagID(tagPath) != "")
    {
        return EEXIST;
    }
    std::string tag = tagPath;
    std::string parentTagID = "0";
    if (tag.find_first_of('/') != std::string::npos)
    {
        std::vector<std::string> parts = splitPathIntoParts(tagPath);
        tag = popBackAndRemove(parts);
        if (!parts.empty())
        {
            parentTagID = getTagID(parts.back());
            if (parentTagID == "")
            {
                return 1; // invalid parent tag
            }
        }
    }
    std::string parentTags = parentTagID + ";";
    macro_bind_text(stmts[CREATE_TAG], tag);
    macro_bind_text(stmts[CREATE_TAG], parentTags);
    dbExecuteSV(stmts[CREATE_TAG]);
    std::string tagID = getTagID(tag);
    if (tagID == "")
    {
        return 1; // invalid tag
    }
    std::vector<std::string> childTagIDs = getChildTagIDs(parentTagID);
    childTagIDs.push_back(tagID);
    updateChildTagIDs(parentTagID, childTagIDs);
    return 0;
}

/**
 * Deletes tag using given tag name or path containing tag name in tag view mode.
 *
 * @param tagPath name of tag to be deleted or path in tag view mode containing it.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::deleteTag(std::string tagPath)
{
    std::string tagID = getTagID(tagPath);
    if (tagID == "")
    {
        return ENOENT;
    }
    std::vector<std::string> fileIDs = getFileIDsUnderTagID(tagID);
    std::vector<std::string> childTagIDs = getChildTagIDs(tagID);
    if (fileIDs.size() != 0 || childTagIDs.size() != 0)
    {
        return ENOTEMPTY;
    }
    std::vector<std::string> parentTagIDs = getParentTagIDs(tagID);
    for (auto parentTagID : parentTagIDs)
    {
        childTagIDs = getChildTagIDs(parentTagID);
        childTagIDs.erase(std::find(childTagIDs.begin(), childTagIDs.end(), tagID));
        updateChildTagIDs(parentTagID, childTagIDs);
    }
    macro_bind_int(stmts[DELETE_TAG], tagID);
    dbExecuteSV(stmts[DELETE_TAG]);
    return 0;
}

/**
 * Updates file IDs of tag specified by tag ID after un/tagging operation.
 *
 * @param tagID tag ID of tag whose file IDs are to be updated.
 * @param fileIDs vector containing tag's file IDs.
 */
void TFSManager::updateTagFileIDs(std::string tagID, std::vector<std::string> fileIDs)
{
    std::string serializedIDs = serializeStrings(fileIDs);
    macro_bind_text(stmts[UPDATE_TAG_FILE_IDS], serializedIDs);
    macro_bind_int(stmts[UPDATE_TAG_FILE_IDS], tagID);
    dbExecuteSV(stmts[UPDATE_TAG_FILE_IDS]);
}

/**
 * Tags given file with given tag.
 *
 * @param fileID file to be tagged specified by its file ID.
 * @param tagID tag ID of tag with which given file is tagged.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::tagSingleFile(std::string fileID, std::string tagID)
{
    std::vector<std::string> filenames = getFilenamesUnderTagID(tagID);
    std::string filename = getFilenameFromID(fileID); // Assume fileID is valid
    auto result = std::find(filenames.begin(), filenames.end(), filename);
    if (result != filenames.end())
    {
        return EEXIST; // filename conflict
    }
    std::vector<std::string> fileIDs = getFileIDsUnderTagID(tagID);
    fileIDs.push_back(fileID);
    updateTagFileIDs(tagID, fileIDs);
    return 0;
}

/**
 * Tags given file(s) with given tag.
 *
 * @param filePath path to file or folder containing files to be tagged.
 * @param tag tag with which given file(s) are tagged.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::tagFiles(std::string filePath, std::string tag)
{
    std::vector<std::string> parts = splitPathIntoParts(filePath);
    std::string name = popBackAndRemove(parts);
    std::string parentFolderID = getFolderID(parts);
    std::string tagID = getTagID(tag);
    if (tagID == "")
    {
        if (createTag(tag) == 1)
        {
            return 1; // invalid tag
        }
        tagID = getTagID(tag);
    }
    if (parentFolderID != "")
    {
        std::string fileID = getFileID(name, parentFolderID);
        if (fileID != "") // tag file
        {
            return tagSingleFile(fileID, tagID);
        }
        // tag all files in folder, not recursive
        std::string folderID = getFolderID(name, parentFolderID);
        if (folderID != "") // tag all files in folder
        {
            int returnValue = 0;
            std::vector<std::string> fileIDs = getFileIDsInFolder(folderID);
            for (auto id : fileIDs)
            {
                if (tagSingleFile(id, tagID) == EEXIST)
                {
                    returnValue = EEXIST; // record conflict, but not fail
                }
            }
            return returnValue;
        }
    }
    return ENOENT;
}

/**
 * Untags given file from given tag.
 *
 * @param fileID file to be untagged specified by its file ID.
 * @param tagID tag ID of tag from which file is untagged.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::untagSingleFile(std::string fileID, std::string tagID)
{
    std::vector<std::string> fileIDs = getFileIDsUnderTagID(tagID);
    auto result = std::find(fileIDs.begin(), fileIDs.end(), fileID);
    if (result == fileIDs.end())
    {
        return ENOENT;
    }
    fileIDs.erase(result);
    updateTagFileIDs(tagID, fileIDs);
    return 0;
}

/**
 * Untags given file(s) from given tag.
 *
 * @param filePath path to file or folder containing files to be untagged.
 * @param tag tag from which given file(s) are untagged.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::untagFiles(std::string filePath, std::string tag)
{
    std::vector<std::string> parts = splitPathIntoParts(filePath);
    std::string name = popBackAndRemove(parts);
    std::string parentFolderID = getFolderID(parts);
    std::string tagID = getTagID(tag);
    if (tagID != "" && parentFolderID != "")
    {
        std::string fileID = getFileID(name, parentFolderID);
        if (fileID != "")
        {
            return untagSingleFile(fileID, tagID);
        }
        // untag all files in folder, not recursive
        std::string folderID = getFolderID(name, parentFolderID);
        if (folderID != "") // untag all files in folder
        {
            int returnValue = 0;
            std::vector<std::string> fileIDs = getFileIDsInFolder(folderID);
            for (auto id : fileIDs)
            {
                if (untagSingleFile(id, tagID) == ENOENT)
                {
                    returnValue = ENOENT; // record missing tagged file, but not fail
                }
            }
            return returnValue;
        }
    }
    return ENOENT;
}

/**
 * Nests given tag inside given parent tag.
 *
 * @param tagID tag ID of tag to be nested.
 * @param parentTagID tag ID of parent tag under which given tag is nested.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::nestTag(std::string tagID, std::string parentTagID)
{
    if (tagID == "" || parentTagID == "")
    {
        return ENOENT;
    }
    std::vector<std::string> childIDs = getChildTagIDs(parentTagID);
    auto result = std::find(childIDs.begin(), childIDs.end(), tagID);
    if (result != childIDs.end())
    {
        return EEXIST;
    }
    else
    {
        childIDs.push_back(tagID);
    }
    std::vector<std::string> parentIDs = getParentTagIDs(tagID);
    result = std::find(parentIDs.begin(), parentIDs.end(), parentTagID);
    if (result != parentIDs.end())
    {
        return EEXIST; // should be caught above but just in case
    }
    else
    {
        parentIDs.push_back(parentTagID);
    }
    // cyclic check
    std::set<std::string> ancestorIDs;
    getAncestorTagIDs(parentTagID, ancestorIDs);
    auto setResult = ancestorIDs.find(tagID);
    if (setResult != ancestorIDs.end())
    {
        return 1; // prevent cyclic nesting
    }
    updateChildTagIDs(parentTagID, childIDs);
    updateParentTagIDs(tagID, parentIDs);
    return 0;
}

/**
 * Unnests given tag from given parent tag.
 *
 * @param tagID tag ID of tag to be unnested.
 * @param parentTagID tag ID of parent tag from which given tag is unnested.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::unnestTag(std::string tagID, std::string parentTagID)
{
    if (tagID == "" || parentTagID == "")
    {
        return ENOENT;
    }
    std::vector<std::string> childIDs = getChildTagIDs(parentTagID);
    auto result = std::find(childIDs.begin(), childIDs.end(), tagID);
    if (result == childIDs.end())
    {
        return ENOENT;
    }
    else
    {
        childIDs.erase(result);
    }
    std::vector<std::string> parentIDs = getParentTagIDs(tagID);
    result = std::find(parentIDs.begin(), parentIDs.end(), parentTagID);
    if (result == parentIDs.end())
    {
        return ENOENT;
    }
    else
    {
        parentIDs.erase(result);
    }
    updateChildTagIDs(parentTagID, childIDs);
    updateParentTagIDs(tagID, parentIDs);
    return 0;
}

/**
 * Gets all tags associated with file specified by file ID.
 *
 * @param fileID file ID of file whose tags are to be found.
 * @return String vector containing tags associated with given file.
 */
std::vector<std::string> TFSManager::getFileTags(std::string fileID)
{
    std::vector<std::string> tags;
    if (getFilenameFromID(fileID) == "")
    {
        return tags;
    }
    std::vector<std::vector<std::string>> results = dbExecuteMR(stmts[GET_FILE_TAGS]);
    for (auto row : results)
    {
        std::vector<std::string> taggedFileIDs = deserializeStrings(row[2]);
        for (auto id : taggedFileIDs)
        {
            if (id == fileID)
            {
                tags.push_back(row[1]);
            }
        }
    }
    return tags;
}

/**
 * Finds file IDs of files tagged with all the given tags.
 *
 * @param tags tags with which files to be found are tagged with.
 * @return String vector containing matching files.
 */
std::vector<std::string> TFSManager::findFileIDsWithTags(std::vector<std::string> tags)
{
    std::vector<std::string> empty;
    std::string tagID = getTagID(tags[0]);
    if (tags.size() == 0 || tagID == "")
    {
        return empty; // invalid tag
    }
    std::vector<std::string> matches = getFileIDsUnderTagID(tagID);
    std::vector<std::string> result;
    std::size_t size = tags.size();
    for (std::size_t i = 1; i < size; i++)
    {
        if ((tagID = getTagID(tags[i])) == "")
        {
            return empty; // invalid tag
        }
        std::vector<std::string> fileIDs = getFileIDsUnderTagID(tagID);
        std::sort(matches.begin(), matches.end());
        std::sort(fileIDs.begin(), fileIDs.end());
        std::set_intersection(matches.begin(), matches.end(),
            fileIDs.begin(), fileIDs.end(),
            std::back_inserter(result));
        matches = result;
        result.empty();
    }
    return matches;
}

/**
 * Finds file IDs of files tagged with any of the given tags.
 *
 * @param tags tags with which files to be found may be tagged with.
 * @return String vector containing matching files.
 */
std::vector<std::string> TFSManager::findFileIDsWithAnyOfTags(std::vector<std::string> tags)
{
    std::vector<std::string> empty;
    std::string tagID;
    if (tags.size() == 0)
    {
        return empty; // invalid tag
    }
    std::set<std::string> matches;
    for (auto tag : tags)
    {
        if ((tagID = getTagID(tag)) == "")
        {
            return empty; // invalid tag
        }
        std::vector<std::string> fileIDs = getFileIDsUnderTagID(tagID);
        for (auto id : fileIDs)
        {
            matches.insert(id);
        }
    }
    return std::vector<std::string>(matches.begin(), matches.end());
}

/**
 * Renames and moves tag from old path to new path by unnesting from old parent tag and nesting
 * under new parent tag.
 *
 * @param oldPath path specifying tag to be moved.
 * @param newPath destination where tag is to be renamed and moved.
 * @return 0 if successful or error value indicating the error.
 */
int TFSManager::renameTaggedPath(std::string oldPath, std::string newPath)
{
    std::string oldParentTagID = getParentTagIDFromPath(oldPath);
    std::string newParentTagID = getParentTagIDFromPath(newPath);
    if (oldParentTagID == "" || newParentTagID == "")
    {
        return ENOENT;
    }
    std::string oldName = getFilename(oldPath);
    std::string newName = getFilename(newPath);
    std::string oldTagID = getTagID(oldName);
    std::string newTagID = getTagID(newName);
    std::string oldFileID = getTaggedFileID(oldParentTagID, oldName);
    std::string newFileID = getTaggedFileID(newParentTagID, newName);
    if (oldFileID != "" && newTagID == "" && newFileID == "")
    {
        if (oldName != newName)
        {
            return 1; // only support changing tags for files
        }
        untagSingleFile(oldFileID, oldParentTagID);
        tagSingleFile(oldFileID, newParentTagID);
        return 0;
    }
    else if (oldTagID != "" && newFileID == "")
    {
        if (oldTagID != newTagID && newTagID != "")
        {
            return 1;
        }
        if (newParentTagID != oldParentTagID)
        {
            unnestTag(oldTagID, oldParentTagID);
            nestTag(oldTagID, newParentTagID);
        }
        if (newTagID == "")
        {
            macro_bind_text(stmts[RENAME_TAGGED_PATH], newName);
            macro_bind_int(stmts[RENAME_TAGGED_PATH], oldTagID);
            dbExecuteSV(stmts[RENAME_TAGGED_PATH]);
        }
        return 0;
    }
    return 1;
}

}

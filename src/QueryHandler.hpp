/**
 * @file QueryHandler.hpp
 * @author Santhosh Ranganathan
 * @brief The header file for the QueryHandler class.
 *
 * @details This file contains the class definition for the QueryHandler class.
 * The QueryHandler class processes command line queries from the user and
 * sends them to the TaggableFS daemon and then receives response which it
 * prints to the user terminal.
 */

#ifndef TFS_QUERYHANDLER_HPP
#define TFS_QUERYHANDLER_HPP

#include "common.hpp"
#include "TFSManager.hpp"

namespace TaggableFS
{

/**
 * This class handles queries to initialize and shutdown TaggableFS and also to
 * perform various tagging operations.
 */
class QueryHandler
{
private:
    /** Stores command line arguments. */
    std::vector<std::string> args;

    /** Sending message queue descriptor. */
    mqd_t txMQ;

    /** Receiving message queue descriptor. */
    mqd_t rxMQ;

    /** Buffer to store messages to be sent/received. */
    char buffer[TFS_MQ_MESSAGE_SIZE];

    /** Check if the TaggableFS daemon is running and responding. */
    bool isTFSManagerResponding;

    /** Check if message queues already exist. */
    bool mqsExist;

    /** Passed on to the TaggableFS daemon to enable logging or not. */
    bool enableLogging;

    /** Passed on to the TaggableFS daemon to mount the filesystem in tag view mode or not. */
    bool tagView;

    void initMQ();
    int initTFS();
    int shutdownTFS();
    std::vector<std::string> queryTFS(std::string query);

public:
    QueryHandler(int argc, char *argv[]);
    int execute();
};

}

#endif
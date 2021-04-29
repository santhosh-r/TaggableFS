/**
 * @file QueryHandler.cpp
 * @author Santhosh Ranganathan
 * @brief The source file for the QueryHandler class.
 *
 * @details This file contains the method definitions for the QueryHandler
 * class.
 */

#include "QueryHandler.hpp"

namespace TaggableFS
{

/**
 * An enum to help with displaying the appropriate help message.
 */
enum QueryHandlerCommands
{
    QH_HELP_START,
    QH_HELP,
    QH_LOG,
    QH_TAG_VIEW,
    QH_INIT,
    QH_EXIT,
    QH_TAG,
    QH_UNTAG,
    QH_NEST,
    QH_UNNEST,
    QH_STATS,
    QH_SEARCH,
    QH_CREATE_TAG,
    QH_DELETE_TAG,
    QH_GET_TAGS,
    QH_HELP_END
};

/**
 * Displays the appropriate help mesesage to the user.
 *
 * @param command command for which help needs to be displayed.
 */
void displayHelp(QueryHandlerCommands command)
{
    std::string helpMessages[] {
        "\e[36mHelp:\e[0m\n",
        "  --help\n"
        "        display this.\n",
        "  --log\n"
        "        log messages to ROOT_DIRECTORY/metadata/log.txt.\n",
        "  --tag-view\n"
        "        open filesystem in read-only mode to browse tags.\n",
        "  --init MOUNT_POINT ROOT_DIRECTORY\n"
        "        launch daemon and mounts FUSE filesystem to the given mount\n"
        "        point and files are stored in root directory.\n",
        "  --shutdown\n"
        "        unmount FUSE filesystem and shutdown daemon.\n",
        "  --tag MOUNTED_PATH TAG\n"
        "        tag the file referenced by mounted path (not in tag view) with the\n"
        "        given tag which will be created if not found. If the path refers to\n"
        "        a folder, all files in it are tagged (non-recursive).\n",
        "  --untag MOUNTED_PATH TAG\n"
        "        untag the file referenced by mounted path (not in tag view) if\n"
        "        tagged with the given tag. If the path refers to a folder, all files\n"
        "        in it are untagged (non-recursive).\n",
        "  --nest TAG PARENT_TAG\n"
        "        nest the given tag inside the given parent tag if both are valid.\n",
        "  --unnest TAG PARENT_TAG\n"
        "        unnest the given tag from the given parent tag if both are valid.\n",
        "  --stats\n"
        "        display stats regarding mounted FUSE filesystem.\n",
        "  --search-tags TAG_1 TAG_2 ... TAG_N [--strict]\n"
        "        search for tagged files with any of the given tags\n"
        "        or with all of them if --strict option is used.\n",
        "  --create-tag TAG\n"
        "        create tag with no children.\n",
        "  --delete-tag TAG\n"
        "        delete tag if it has no children.\n",
        "  --get-tags FILE_PATH\n"
        "        display all tags current used to tag the file.\n"
    };

    int start = command;
    int end = start + 1;
    if (command == QH_HELP)
    {
        end = QH_HELP_END;
        std::cout << "\e[35mTaggableFS\e[0m\n\n";
    }
    std::cout << helpMessages[QH_HELP_START];
    for (auto i = start; i < end; i++)
    {
        std::cout << helpMessages[i] << std::endl;
    }
}

/**
 * Constructor for the QueryHandler class.
 *
 * @param argc number of command line arguments.
 * @param argv command line arguments.
 */
QueryHandler::QueryHandler(int argc, char *argv[]) : enableLogging(false), tagView(false)
{
    args = std::vector<std::string>(argv, argv + argc);
    auto loggingOption = std::find(args.begin(), args.end(), "--log");
    if (loggingOption != args.end())
    {
        enableLogging = true;
        args.erase(loggingOption);
    }
    auto tagViewOption = std::find(args.begin(), args.end(), "--tag-view");
    if (tagViewOption != args.end())
    {
        tagView = true;
        args.erase(tagViewOption);
    }

    initMQ();
}

/**
 * Initilaizes message queues to send/receive messages to and from the TaggableFS daemon. If
 * they already exist, it checks if the daemon is responding.
 */
void QueryHandler::initMQ()
{
    mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = TFS_MQ_MAX_MESSAGES;
    attr.mq_msgsize = TFS_MQ_MESSAGE_SIZE;
    attr.mq_curmsgs = 0;

    isTFSManagerResponding = mqsExist = false;
    txMQ = mq_open("/tfs_managermq", O_WRONLY, 0660, &attr);
    if (txMQ != -1)
    {
        rxMQ = mq_open("/tfs_querymq", O_RDONLY, 0660, &attr);
        if (rxMQ == -1)
        {
            perror("ERROR: QueryHandler mq_open() failed");
            return;
        }
        mqsExist = true;

        timespec time;
        clock_gettime(CLOCK_REALTIME, &time);
        time.tv_sec += 1;
        serializeMessage("QH_TEST", buffer);
        if (mq_timedsend(txMQ, buffer, TFS_MQ_MESSAGE_SIZE, 0, &time) == -1) // working
        {
            perror("ERROR: QueryHandler mq_timedsend() failed");
            return;
        }

        clock_gettime(CLOCK_REALTIME, &time);
        time.tv_sec += 1;
        if (mq_timedreceive(rxMQ, buffer, TFS_MQ_MESSAGE_SIZE, NULL, &time) == -1)
        {
            perror("ERROR: QueryHandler mq_timedreceive() failed");
            return;
        }
        isTFSManagerResponding = true;
    }
}

/**
 * Initializes the TaggableFS daemon.
 *
 * @return Result of the attempt to initialize the daemon.
 */
int QueryHandler::initTFS()
{
    if (isTFSManagerResponding == true)
    {
        std::cerr << "ERROR: TaggableFS is already running." << std::endl;
        return 1;
    }
    if (mqsExist == true)
    {
        std::cerr << "ERROR: TaggableFS not shutdown properly or running "
            "but not responsive." << std::endl;
        return 1;
    }

    char *buf = new char[PATH_MAX];
    // check if mount point and root directory are valid
    std::string mountPoint = realpath(args[2].c_str(), buf);
    std::string rootDirectory = realpath(args[3].c_str(), buf);
    delete[] buf;
    if (mountPoint == "" || rootDirectory == "")
    {
        std::cerr << "ERROR: Invalid mount point and/or root directory." << std::endl;
        return 1;
    }
    std::string programName = args[0];

    std::cout << "Initializing TaggableFS..." << std::endl;
    TFSManager tfsManager(mountPoint, rootDirectory, programName, enableLogging, tagView);
    int returnValue =  tfsManager.init();
    initMQ(); // reinitialize message queues.
    if (returnValue == 0)
    {
        std::cout << "TaggableFS initialized." << std::endl;
    }
    else
    {
        std::cerr << "ERROR: TaggableFS could not be initialized." << std::endl;
        std::cerr << strerror(returnValue) << std::endl;
    }

    return returnValue;
}

/**
 * Shuts down the TaggableFS daemon.
 *
 * @return 0 if succeeded in shutting down the daemon.
 */
int QueryHandler::shutdownTFS()
{
    if (mqsExist == false)
    {
        std::cerr << "ERROR: Message queues don't exist." << std::endl;
        std::cout << "TaggableFS might have already been shutdown." << std::endl;
        return 0;
    }
    bool messageSent = false;
    if (isTFSManagerResponding == true)
    {
        timespec time;
        clock_gettime(CLOCK_REALTIME, &time);
        time.tv_sec += 1;
        serializeMessage("QH_EXIT", buffer);
        messageSent = (mq_timedsend(txMQ, buffer, TFS_MQ_MESSAGE_SIZE, 0, &time) != -1);
        if (messageSent == false)
        {
            perror("ERROR: QueryHandler mq_timedsend() failed");
        }
    }
    if (messageSent == false)
    {
        std::cout << "TaggableFS hanging or not shutdown properly." << std::endl;
        mq_unlink("/tfs_fusemq");
        mq_unlink("/tfs_querymq");
        mq_unlink("/tfs_managermq");
        std::cout << "Cleaned up mqueues." << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Request to shutdown sent." << std::endl;

    return 0;
}

/**
 * Processes the command line queries.
 *
 * @return 0 if successfully processed query else 1.
 */
int QueryHandler::execute()
{
    int numberOfArguments = args.size() - 2;
    std::string command = args[1];
    if (command == "--help")
    {
        displayHelp(QH_HELP);
        return 0;
    }
    else if (command == "--init")
    {
        if (numberOfArguments != 2)
        {
            std::cerr << "ERROR: Invalid arguments.\n";
            displayHelp(QH_INIT);
            return 1;
        }
        return initTFS();
    }
    else if (command == "--shutdown")
    {
        if (numberOfArguments != 0)
        {
            std::cerr << "ERROR: Invalid arguments.\n";
            displayHelp(QH_EXIT);
            return 1;
        }
        return shutdownTFS();
    }
    else if (command == "--tag")
    {
        if (numberOfArguments != 2)
        {
            std::cerr << "ERROR: Invalid arguments.\n";
            displayHelp(QH_TAG);
            return 1;
        }
        std::vector<std::string> response = queryTFS("QH_TAG " + args[2] + "," + args[3]);
        std::cout << "RESPONSE: " << response[0] << std::endl;
        return 0;
    }
    else if (command == "--untag")
    {
        if (numberOfArguments != 2)
        {
            std::cerr << "ERROR: Invalid arguments.\n";
            displayHelp(QH_UNTAG);
            return 1;
        }
        std::vector<std::string> response = queryTFS("QH_UNTAG " + args[2] + "," + args[3]);
        std::cout << "RESPONSE: " << response[0] << std::endl;
        return 0;
    }
    else if (command == "--nest")
    {
        if (numberOfArguments != 2)
        {
            std::cerr << "ERROR: Invalid arguments.\n";
            displayHelp(QH_NEST);
            return 1;
        }
        std::vector<std::string> response = queryTFS("QH_NEST " + args[2] + "," + args[3]);
        std::cout << "RESPONSE: " << response[0] << std::endl;
        return 0;
    }
    else if (command == "--unnest")
    {
        if (numberOfArguments != 2)
        {
            std::cerr << "ERROR: Invalid arguments.\n";
            displayHelp(QH_UNNEST);
            return 1;
        }
        std::vector<std::string> response = queryTFS("QH_UNNEST " + args[2] + "," + args[3]);
        std::cout << "RESPONSE: " << response[0] << std::endl;
        return 0;
    }
    else if (command == "--stats")
    {
        if (numberOfArguments != 0)
        {
            std::cerr << "ERROR: Invalid arguments.\n";
            displayHelp(QH_STATS);
            return 1;
        }
        std::vector<std::string> response = queryTFS("QH_STATS");
        std::cout << "RESPONSE: " << response[0] << std::endl;
        return 0;
    }
    else if (command == "--search-tags")
    {
        std::vector<std::string> arguments(args.begin() + 2, args.end());
        auto strictOption = std::find(arguments.begin(), arguments.end(), "--strict");
        int strict = 0;
        if (strictOption != arguments.end())
        {
            strict = 1;
            arguments.erase(strictOption);
        }
        if (arguments.size() == 0)
        {
            std::cerr << "ERROR: No tags given.\n";
            displayHelp(QH_SEARCH);
            return 1;
        }
        std::string query = "QH_SEARCH " + std::to_string(strict)
            + "," + serializeStrings(arguments);
        std::vector<std::string> response = queryTFS(query);
        std::cout << "SEARCH RESULTS (Strict Search: " << ((strict==0) ? "OFF" : "ON") << "):\n";
        if (response[0] == "")
        {
            std::cout << "\e[31mNo files Found\e[0m";
        }
        for (auto result : response)
        {
            std::cout << result << std::endl;
        }
        return 0;
    }
    else if (command == "--create-tag")
    {
        if (numberOfArguments != 1)
        {
            std::cerr << "ERROR: Invalid arguments.\n";
            displayHelp(QH_CREATE_TAG);
            return 1;
        }
        std::vector<std::string> response = queryTFS("QH_CREATE_TAG " + args[2]);
        std::cout << "RESPONSE: " << response[0] << std::endl;
        return 0;
    }
    else if (command == "--delete-tag")
    {
        if (numberOfArguments != 1)
        {
            std::cerr << "ERROR: Invalid arguments.\n";
            displayHelp(QH_DELETE_TAG);
            return 1;
        }
        std::vector<std::string> response = queryTFS("QH_DELETE_TAG " + args[2]);
        std::cout << "RESPONSE: " << response[0] << std::endl;
        return 0;
    }
    else if (command == "--get-tags")
    {
        if (numberOfArguments != 1)
        {
            std::cerr << "ERROR: Invalid arguments.\n";
            displayHelp(QH_GET_TAGS);
            return 1;
        }
        std::vector<std::string> response = queryTFS("QH_GET_TAGS " + args[2]);
        if (response[0] == "Invalid")
        {
            std::cerr << "ERROR: Invalid path given.\n";
            return 1;
        }
        std::vector<std::string> tags = deserializeStrings(response[0]);
        std::cout << "TAGS: " << std::endl;
        if (tags.size() == 0)
        {
            std::cout << "\e[31mNo Tags Found\e[0m" << std::endl;
        }
        for (auto tag : tags)
        {
            std::cout << tag << std::endl;
        }
        return 0;
    }
    std::cerr << "ERROR: Invalid command and arguments. Use --help to see commands.\n";
    return 1;
}

/**
 * Sends query to the TaggableFS daemon and receives single or multipart response.
 *
 * @param query query to be sent.
 * @return Response from the daemon.
 */
std::vector<std::string> QueryHandler::queryTFS(std::string query)
{
    if (isTFSManagerResponding == false)
    {
        std::cerr << "ERROR: TaggableFS not running or unreachable." << std::endl;
        exit(EXIT_FAILURE);
    }

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

}

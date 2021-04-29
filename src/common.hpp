/**
 * @file common.hpp
 * @author Santhosh Ranganathan
 * @brief The header file for commonly used code.
 *
 * @details This file contains common includes, utility classes and functions.
 */

#ifndef TFS_COMMON_HPP
#define TFS_COMMON_HPP

#include <iostream>
#include <vector>
#include <cstring>
#include <mqueue.h>
#include <limits.h>
#include <algorithm>

/** Maximum number of messages stored in message queue. */
#define TFS_MQ_MAX_MESSAGES 10

/** Size of message buffer used in message queue functions. */
#define TFS_MQ_MESSAGE_SIZE 6144

namespace TaggableFS
{

/**
 * A plain old/C-compatible data type used for sending and receiving messages.
 */
struct Message
{
    /** Boolean to indicate if message is complete or not. */
    bool complete;
    /** Buffer storing message sent/received. */
    char content[TFS_MQ_MESSAGE_SIZE - 16];
};

void serializeMessage(const char *content, char (&data)[TFS_MQ_MESSAGE_SIZE], bool complete = true);
Message deserializeMessage(char (&data)[TFS_MQ_MESSAGE_SIZE]);

std::string serializeStrings(std::vector<std::string> &ids, char separator=';');
std::vector<std::string> deserializeStrings(std::string serializedIDs, char separator=';');
std::string getFilename(std::string path);
std::vector<std::string> splitAtFirstOccurance(std::string source, char character=' ');
std::vector<std::string> splitPathIntoParts(std::string path);
std::string popBackAndRemove(std::vector<std::string> &parts);

}

#endif


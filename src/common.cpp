/**
 * @file common.cpp
 * @author Santhosh Ranganathan
 * @brief The source file for commonly used code.
 *
 * @details This file contains the definitions for the common utility classes
 * and functions.
 */

#include "common.hpp"

namespace TaggableFS
{

/**
 * Serializes Message struct into a character array sent using POSIX message queues.
 *
 * @param content part of or entire message string to be sent.
 * @param data bufffer to save message struct to.
 * @param complete boolean to indicate if message is complete.
 */
void serializeMessage(const char *content, char (&data)[TFS_MQ_MESSAGE_SIZE], bool complete)
{
    Message m = {complete, ""};
    strncpy(m.content, content, TFS_MQ_MESSAGE_SIZE - 16);
    memcpy(data, &m, sizeof m);
}

/**
 * Deserializes character array received using POSIX message queues into Message struct.
 *
 * @param data bufffer where message struct was saved to.
 * @return Deserialized Message struct.
 */
Message deserializeMessage(char (&data)[TFS_MQ_MESSAGE_SIZE])
{
    Message m;
    memcpy(&m, data, sizeof m);
    return m;
}

/**
 * Serializes an array of strings eg. ids into a single string using the given separator.
 *
 * @param ids array of ids to be serialized.
 * @param separator separator to be used to separate the array elements.
 * @return Single string containing all the given ids.
 */
std::string serializeStrings(std::vector<std::string> &ids, char separator)
{
    std::string serializedIDs = "";
    for (auto id : ids)
    {
        if (id != "")
        {
            serializedIDs += id + separator;
        }
    }
    return serializedIDs;
}

/**
 * Deserializes a single string into an array of strings using the given separator.
 *
 * @param serializedIDs string containing ids.
 * @param separator separator to be used to separate the array elements.
 * @return Vector of deserialized ids.
 */
std::vector<std::string> deserializeStrings(std::string serializedIDs, char separator)
{
    std::vector<std::string> ids;
    for (std::size_t begin = 0, end = 1; end < serializedIDs.length(); end++)
    {
        if (serializedIDs[end] == separator)
        {
            ids.push_back(serializedIDs.substr(begin, end - begin));
            begin = end + 1;
        }
    }
    return ids;
}

/**
 * Extracts the filename from the given path.
 *
 * @param path path from which filename is to be extracted.
 * @return Filename extracted from the given path.
 */
std::string getFilename(std::string path)
{
    return path.substr(path.find_last_of('/') + 1);
}

/**
 * Splits the given source string at first occurance of the given character.
 *
 * @param source source string.
 * @param character character at which source string is split.
 * @return Two substrings of the source string split at the given character.
 */
std::vector<std::string> splitAtFirstOccurance(std::string source, char character)
{
    std::vector<std::string> results;
    auto position = source.find_first_of(character);
    if (position == std::string::npos)
    {
        results.push_back(source);
    }
    else
    {
        results.push_back(source.substr(0, position));
        results.push_back(source.substr(position + 1));
    }
    return results;
}

/**
 * Splits the given path into its component parts.
 *
 * @param path path which to be split.
 * @return Vector of component parts of the given path.
 */
std::vector<std::string> splitPathIntoParts(std::string path)
{
    std::vector<std::string> parts;
    if (path[0] == '/')
    {
        std::string remaining = path.substr(1);
        std::size_t position = 0;
        while (!remaining.empty() && position != std::string::npos)
        {
            position = remaining.find_first_of('/');
            std::string part = remaining.substr(0, position);
            if (part != "")
            {
                parts.push_back(part);
            }
            remaining = remaining.substr(position + 1);
        }
    }
    return parts;
}

/**
 * Returns the last element of a vector and removes it.
 *
 * @param parts vector from which the last element is popped.
 * @return Last element that was popped from the vector.
 */
std::string popBackAndRemove(std::vector<std::string> &parts)
{
    if (parts.empty())
    {
        return "";
    }
    std::string lastElement = parts.back();
    parts.pop_back();
    return lastElement;
}

}

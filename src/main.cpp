/**
 * @file main.cpp
 * @author Santhosh Ranganathan
 * @brief The entry point for TaggableFS.
 *
 * @details This file contains the entry point main() function which passes the
 * command line queries to QueryHandler.
 */

#include "TaggableFS.hpp"

/**
 * The entry point function for TaggableFS.
 *
 * @param argc number of arguments.
 * @param argv arguments.
 * @return Result from executing QueryHandler.
 */
int main(int argc, char *argv[])
{
    TaggableFS::QueryHandler qh(argc, argv);
    return qh.execute();
}

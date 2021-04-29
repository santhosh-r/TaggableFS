/**
 * @file TaggableFS.hpp
 * @author Santhosh Ranganathan
 * @brief The default header file for TaggableFS.
 */

/**
 * @mainpage TaggableFS - Exploring a tag based filesystem
 * TaggableFS was created to explore how a tag based filesystem might
 * work and how it could interoperate with Linux which uses a hierarchical
 * filesystem. There are two modes in which the filesystem can be mounted:
 * a default mode and a tag view mode. In the default mode, a user can copy
 * files into the filesystem and can later perform tagging operations via
 * command line queries. In the tag view mode, the user can browse the tagged
 * files through their tags represented as folders and also create and move
 * tags around while the files themselves are read only.
 *
 * This software was created as part of the course CSI 680 "Computer Science
 * Research" to fulfill the project requirement for my MS degree in Computer
 * Science at UAlbany. The idea to create a tag based filesystem was
 * suggested to me by my project supervisor Michael Phipps.
 *
 * Doxygen has been allowed to extract private members when generating
 * documentation.
 */

#ifndef TFS_TAGGABLEFS_HPP
#define TFS_TAGGABLEFS_HPP

#include "QueryHandler.hpp"

#endif
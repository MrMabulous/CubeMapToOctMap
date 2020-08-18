/*
 * Copyright(c) 2020 Matthias Bühlmann, Mabulous GmbH. http://www.mabulous.com
*/

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string>
#include <vector>

// Removes leading and trailing whitespace from string.
std::string trim(const std::string& s);

// Splits string at delimiter into array.
std::vector<std::string> split(const std::string& str, const std::string& delimiter);

// Converts string to lower case.
std::string toLower(const std::string& s);

#endif  // STRING_UTILS_H
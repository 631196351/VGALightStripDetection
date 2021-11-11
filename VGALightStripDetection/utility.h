#pragma once

#include <string>

std::string RunCmd(std::string strCmd);

void shutdownAfter(unsigned int secons, bool reboot = false);
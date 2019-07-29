#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <ncurses.h>
#include <time.h>
#include <sstream>
#include <iomanip>
#include "Util.h"
#include "SysInfo.h"
#include "ProcessContainer.h"

using namespace std;

std::string ProcessParser::getVmSize(std::string pid) {
  std::string name = "VmData";
  std::string line;
  std::ifstream stream;
  Util::getStream(Path::basePath() + pid + Path::statusPath(), stream);
  float result = 0;
  while(std::getline(stream, line)) {
    if (line.compare(0, name.size(), name) == 0) {
      std::istringstream iss(line);
      std::istream_iterator<string> beg(iss), end;
      std::vector<string> values(beg, end);
      result = (std::stof(values[1])/float(1024*1024));
      break;
    }
  }
  return std::to_string(result);
}

std::string ProcessParser::getCpuPercent(std::string pid) {
  std::string line;
  float result;
  std::ifstream stream;
  Util::getStream(Path::basePath() + pid + Path::statPath(), stream);
  getline(stream, line);
  istringstream iss(line);
  istream_iterator<std::string> beg(iss), end;
  vector<std::string> values(beg, end);

  // Formula copied from Course material
  float utime = stof(ProcessParser::getProcUpTime(pid));
  float stime = stof(values[14]);
  float cutime = stof(values[15]);
  float cstime = stof(values[16]);
  float starttime = stof(values[21]);
  float uptime = ProcessParser::getSysUpTime();
  float freq = sysconf(_SC_CLK_TCK);
  float total_time = utime + stime + cutime + cstime;
  float seconds = uptime - (starttime/freq);
  result = 100.0*((total_time/freq)/seconds);

  return std::to_string(result);
}

std::string ProcessParser::getProcUpTime(std::string pid) {
  string line;
  string value;
  float result;
  ifstream stream;
  Util::getStream((Path::basePath() + pid + "/" +  Path::statPath()), stream);
  getline(stream, line);
  string str = line;
  istringstream buf(str);
  istream_iterator<string> beg(buf), end;
  vector<string> values(beg, end); // done!
  // Using sysconf to get clock ticks of the host machine
  return to_string(float(stof(values[13])/sysconf(_SC_CLK_TCK)));
}

long int ProcessParser::getSysUpTime()
{
    string line;
    ifstream stream;
    Util::getStream((Path::basePath() + Path::upTimePath()), stream);
    getline(stream,line);
    istringstream buf(line);
    istream_iterator<string> beg(buf), end;
    vector<string> values(beg, end);
    return stoi(values[0]);
}

vector<string> ProcessParser::getPidList()
{
    DIR* dir;
    // Basically, we are scanning /proc dir for all directories with numbers as their names
    // If we get valid check we store dir names in vector as list of machine pids
    vector<string> container;
    if(!(dir = opendir("/proc")))
        throw std::runtime_error(std::strerror(errno));

    while (dirent* dirp = readdir(dir)) {
        // is this a directory?
        if(dirp->d_type != DT_DIR)
            continue;
        // Is every character of the name a digit?
        if (all_of(dirp->d_name, dirp->d_name + std::strlen(dirp->d_name), [](char c){ return std::isdigit(c); })) {
            container.push_back(dirp->d_name);
        }
    }
    //Validating process of directory closing
    if(closedir(dir))
        throw std::runtime_error(std::strerror(errno));
    return container;
}

string ProcessParser::getCmd(string pid)
{
    string line;
    ifstream stream;
    Util::getStream((Path::basePath() + pid + Path::cmdPath()), stream);
    std::getline(stream, line);
    return line;
}

string ProcessParser::getProcUser(string pid)
{
    string line;
    string name = "Uid:";
    string result ="";
    ifstream stream;
    Util::getStream((Path::basePath() + pid + Path::statusPath()), stream);
    // Getting UID for user
    while (std::getline(stream, line)) {
        if (line.compare(0, name.size(),name) == 0) {
            istringstream buf(line);
            istream_iterator<string> beg(buf), end;
            vector<string> values(beg, end);
            result =  values[1];
            break;
        }
    }
    Util::getStream("/etc/passwd", stream);
    name =("x:" + result);
    // Searching for name of the user with selected UID
    while (std::getline(stream, line)) {
        if (line.find(name) != std::string::npos) {
            result = line.substr(0, line.find(":"));
            return result;
        }
    }
    return "";
}

int ProcessParser::getNumberOfCores()
{
    // Get the number of host cpu cores
    string line;
    string name = "cpu cores";
    ifstream stream;
    Util::getStream((Path::basePath() + "cpuinfo"), stream);
    while(getline(stream, line)) {
      if (line.compare(0, name.size(), name) == 0) {
        istringstream iss(line);
        istream_iterator<string> beg(iss), end;
        vector<string> values(beg, end);
        return stoi(values[3]);
      }
    }
    return 0;
}
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <ncurses.h>
#include <time.h>
#include <sstream>
#include <iomanip>
#include "util.h"
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

char* getCString(std::string str){
    char * cstr = new char [str.length()+1];
    std::strcpy (cstr, str.c_str());
    return cstr;
}
void writeSysInfoToConsole(SysInfo sys, WINDOW* sys_win){
    sys.setAttributes();

    mvwprintw(sys_win,2,2,getCString(( "OS: " + sys.getOSName())));
    mvwprintw(sys_win,3,2,getCString(( "Kernel version: " + sys.getKernelVersion())));
    mvwprintw(sys_win,4,2,getCString( "CPU: "));
    wattron(sys_win,COLOR_PAIR(1));
    wprintw(sys_win,getCString(Util::getProgressBar(sys.getCpuPercent())));
    wattroff(sys_win,COLOR_PAIR(1));
    mvwprintw(sys_win,5,2,getCString(( "Other cores:")));
    wattron(sys_win,COLOR_PAIR(1));
    std::vector<std::string> val = sys.getCoresStats();
    for(int i=0;i<val.size();i++){
     mvwprintw(sys_win,(6+i),2,getCString(val[i]));
    }
    wattroff(sys_win,COLOR_PAIR(1));
    mvwprintw(sys_win,10,2,getCString(( "Memory: ")));
    wattron(sys_win,COLOR_PAIR(1));
    wprintw(sys_win,getCString(Util::getProgressBar(sys.getMemPercent())));
    wattroff(sys_win,COLOR_PAIR(1));
    mvwprintw(sys_win,11,2,getCString(( "Total Processes:" + sys.getTotalProc())));
    mvwprintw(sys_win,12,2,getCString(( "Running Processes:" + sys.getRunningProc())));
    mvwprintw(sys_win,13,2,getCString(( "Up Time: " + Util::convertToTime(sys.getUpTime()))));
    wrefresh(sys_win);
}

void getProcessListToConsole(std::vector<string> processes,WINDOW* win){

    wattron(win,COLOR_PAIR(2));
    mvwprintw(win,1,2,"PID:");
    mvwprintw(win,1,9,"User:");
    mvwprintw(win,1,16,"CPU[%%]:");
    mvwprintw(win,1,26,"RAM[MB]:");
    mvwprintw(win,1,35,"Uptime:");
    mvwprintw(win,1,44,"CMD:");
    wattroff(win, COLOR_PAIR(2));
    for(int i=0; i< processes.size();i++){
        mvwprintw(win,2+i,2,getCString(processes[i]));
   }
}
void printMain(SysInfo sys,ProcessContainer procs){
	initscr();			/* Start curses mode 		  */
    noecho(); // not printing input values
    cbreak(); // Terminating on classic ctrl + c
    start_color(); // Enabling color change of text
    int yMax,xMax;
    getmaxyx(stdscr,yMax,xMax); // getting size of window measured in lines and columns(column one char length)
	WINDOW *sys_win = newwin(17,xMax-1,0,0);
	WINDOW *proc_win = newwin(15,xMax-1,18,0);


    init_pair(1,COLOR_BLUE,COLOR_BLACK);
    init_pair(2,COLOR_GREEN,COLOR_BLACK);
    int counter = 0;
    while(1){
    box(sys_win,0,0);
    box (proc_win,0,0);
    procs.refreshList();
    std::vector<std::vector<std::string>> processes = procs.getList();
    writeSysInfoToConsole(sys,sys_win);
    getProcessListToConsole(processes[counter],proc_win);
    wrefresh(sys_win);
    wrefresh(proc_win);
    refresh();
    sleep(1);
    if(counter ==  (processes.size() -1)){
        counter = 0;
    }
    else {
        counter ++;
    }
    }
	endwin();
}
int main( int   argc, char *argv[] )
{
 //Object which contains list of current processes, Container for Process Class
    ProcessContainer procs;
// Object which containts relevant methods and attributes regarding system details
    SysInfo sys;
    //std::string s = writeToConsole(sys);
    printMain(sys,procs);
    return 0;
}

#include "Process.h"

using namespace std;

void Process::setPid(int pid){
    this->pid = pid;
}
string Process::getPid()const {
    return this->pid;
}

string Process::getProcess()
{
    if(!ProcessParser::isPidExisting(this->pid))
        return "";
    this->mem = ProcessParser::getVmSize(this->pid);
    this->up_time = ProcessParser::getProcUpTime(this->pid);
    this->cpu = ProcessParser::getCpuPercent(this->pid);

    return (this->pid + "   "
                    + this->user
                    + "   "
                    + this->mem.substr(0,5)
                    + "     "
                    + this->cpu.substr(0,5)
                    + "     "
                    + this->up_time.substr(0,5)
                    + "    "
                    + this->cmd.substr(0,30)
                    + "...");
}



#ifndef LOG_H
#define LOG_H

#include<string>
#include<initializer_list>
#include<ctime>
#include<thread>
#include<fstream>
#include"LogQueue.hpp"

using std::string;
using std::thread;
using std::ofstream;
using std::initializer_list;

using LOG = initializer_list<string>;

class Log
{
public:
    enum LOG_LEVEL {
        DEBUG = 0,
        INFO,
        WARN,
        ERROR
    };

private:
    Log();
    ~Log() = default;

    void threadLoop();
    void getCurDate(string& recondDate, string& recondTime);

public:
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    static Log* getInstance();
    void initLog(size_t maxLine = 50000, bool isAsync = false, size_t queueSize = 0);
    void print(LOG_LEVEL level, initializer_list<string>&& str);
    void freeLog();

private:
    string m_recordTime;
    string m_recordDate;
    string m_fileName;

    size_t m_lineCount;
    size_t m_maxLine;
    size_t m_page;

    ofstream m_output;
    bool m_quitLog;
    
private:
    bool m_isAsync;
    shared_ptr<LogQueue<string>> m_queue;
    shared_ptr<thread> m_thread;
};

#define DEBUG(text) Log::getInstance()->print(Log::DEBUG, text)
#define INFO(text) Log::getInstance()->print(Log::INFO, text)
#define WARN(text) Log::getInstance()->print(Log::WARN, text)
#define ERROR(text) Log::getInstance()->print(Log::ERROR, text)

#endif
#include"Log.hpp"

Log* Log::getInstance()
{
    static Log instance;
    return &instance;
}

Log::Log():m_lineCount(0), 
           m_maxLine(0),
           m_page(1),
           m_isAsync(false),
           m_quitLog(false)
           {}

void Log::initLog(size_t maxLine, bool isAsync, size_t queueSize)
{
    if (maxLine <= 0 || queueSize < 0)
    {
        std::cerr << "Unacceptable number to initLog!\n";
        exit(-1);
    }

    m_isAsync = isAsync;
    m_maxLine = maxLine;
    getCurDate(m_recordDate, m_recordTime);
    m_fileName = m_recordDate + "-1.log";

    if (isAsync && queueSize > 0)
    {
        shared_ptr<LogQueue<string>> ret(new LogQueue<string>(queueSize));
        m_queue = ret;
        m_thread = std::make_shared<thread>(thread(&Log::threadLoop, this));
    }

}

void Log::getCurDate(string& recondDate, string& recondTime)
{
    time_t now;
    time(&now);
    struct tm temp;
    localtime_r(&now, &temp);

    recondDate = std::to_string(temp.tm_year + 1900);
    
    if (temp.tm_mon < 9)
        recondDate += "0";
    recondDate += std::to_string(temp.tm_mon + 1);

    if (temp.tm_mday < 10)
        recondDate += "0";
    recondDate += std::to_string(temp.tm_mday);

    recondTime.clear();
    if (temp.tm_hour < 10)
        recondTime += "0";
    recondTime += std::to_string(temp.tm_hour) + ":";

    if (temp.tm_min < 10)
        recondTime += "0";
    recondTime += std::to_string(temp.tm_min) + ":";

    if (temp.tm_sec < 10)
        recondTime += "0";
    recondTime += std::to_string(temp.tm_sec);
}

void Log::print(LOG_LEVEL level, initializer_list<string>&& str)
{
    string text, temp;

    switch (level)
    {
        case DEBUG:
        {
            temp = "[DEBUG]";
            break;
        }
        case INFO:
        {
            temp = "[INFO]";
            break;
        }
        case WARN:
        {
            temp = "[WARN]";
            break;
        }
        case ERROR:
        {
            temp = "[ERROR]";
            break;
        }
    }

    string recondDate;
    getCurDate(recondDate, m_recordTime);
    text = temp + '[' + recondDate + ' ' + m_recordTime + "] ";

    for (auto& i : str)
        text += i;
    
    if (recondDate != m_recordDate)
    {
        m_recordDate = recondDate;
        m_lineCount = 0;
        m_page = 1;
        m_fileName = m_recordDate + "-1.log";
    }

    if (m_isAsync)
    {
        auto log = std::make_shared<string>(string(text));
        m_queue->addTask(log);
        return;
    }

    ++m_lineCount;
    int val = m_maxLine - m_lineCount;
    if (val <= 0)
    {
        ++m_page;
        m_lineCount = 1;
        m_fileName = m_recordDate + '-' + std::to_string(m_page) + ".log";
    }

    m_output.open(m_fileName, std::ios_base::app);
    if (m_output.is_open())
    {
        m_output << text << std::endl;
        m_output.close();
    }
}

void Log::threadLoop()
{
    while (!m_quitLog)
    {
        auto ret = m_queue->getTask();
        
        if (!ret)
            continue;
        
        ++m_lineCount;
        int val = m_maxLine - m_lineCount;
        if (val <= 0)
        {
            ++m_page;
            m_lineCount = 1;
            m_fileName = m_recordDate + '-' + std::to_string(m_page) + ".log";
        }

        m_output.open(m_fileName, std::ios_base::app);
        if (m_output.is_open())
        {
            m_output << *ret << std::endl;
            m_output.close();
        }
    }
}

void Log::freeLog()
{
    if (m_output.is_open())
        m_output.close();
    
    if (m_queue)
    {
        m_queue->clearTask();
    }

    if (m_thread)
    {
        m_quitLog = true;
        m_thread->join();
    }
}
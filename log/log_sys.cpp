#include"log_sys.hpp"

/*
* 函数功能： 线程安全下获得LogSys单例模式的指针，利于后续调用
* 输入参数： NULL
* 返回值：  LogSys实例化对象的指针【单例模式，指针不会更改】
*/
LogSys* LogSys::getInstance()
{
    static LogSys s_instance;/*运用c++11中static的线程安全的新特性，在第一次调用时不会出现线程竞争的情况*/
    return &s_instance;
}

LogSys::LogSys():m_lineCount(0),
                 m_maxLineSize(0),
                 m_logPage(1),
                 m_mutex(),
                 m_logIsAsync(false),
                 m_blockSize(0),
                 m_blockQueue(NULL),
                 m_logThread(NULL){}

LogSys::~LogSys()
{
    if (m_outputFile.is_open())
    {
        m_outputFile.close();
    }

    if (m_logThread != NULL)
    {
        m_logThread->join();
        delete m_logThread;
        m_logThread = NULL;
    }

    if (m_blockQueue != NULL)
    {
        m_blockQueue->clearTaskQueue();
        delete m_blockQueue;
        m_blockQueue = NULL;
    }    
}

/*
* 函数功能： 选择日志输出的模式，同步/异步
* 输入参数： maxLineSize 单个日志文件最大行数
* 输入参数： logIsAsync 是否开启异步模式， true为开启
* 输入参数： blockSize 异步模式下阻塞队列大小
* 返回值：  void
*/
void LogSys::initLogSys(int maxLineSize, bool logIsAsync, int blockSize)
{
    assert(maxLineSize > 0 || blockSize >= 0);
    m_maxLineSize = maxLineSize;
    getDate(m_dateRecord, m_timeRecord);
    m_fileName = m_dateRecord + "-1.log";

    if (logIsAsync && blockSize > 0)
    {
        m_blockSize = blockSize;
        m_blockQueue = new BlockQueue<string*>(m_blockSize);
        m_logThread = new std::thread(&LogSys::threadLoop, this);
    }
}

/*
* 函数功能： 获取当前运行的时间日期，以string的形式传出
* 输入参数： NULL
* 输出参数： date 当前运行的日期，string对象格式为YYYYMMDD
* 输出参数： clock 当前运行的时间，string对象格式为HH：MM：SS
* 返回值：  void
*/
void LogSys::getDate(string& date, string& clock)
{
    time_t now;
    time(&now);
    struct tm temp;
    localtime_r(&now, &temp);

    date = std::to_string(temp.tm_year + 1900);

    if (temp.tm_mon < 9)
        date += '0';
    date += std::to_string(temp.tm_mon + 1);

    if (temp.tm_mday < 10)
        date += '0';
    date += std::to_string(temp.tm_mday);

    clock.clear();
    if (temp.tm_hour < 10)
        clock = '0';
    clock += std::to_string(temp.tm_hour) + ':';
    
    if (temp.tm_min < 10)
        clock += '0';
    clock = clock + std::to_string(temp.tm_min) + ':';

    if (temp.tm_sec < 10)
        clock += '0';
    clock = clock + std::to_string(temp.tm_sec);
}

/*
* 函数功能： 将initializer_list<string>对象以文件读写模式输出,依据是否异步选择不同方法
* 输入参数： level 枚举类型LOG_LEVEL下的四种模式
* 输入参数： str 列表初始化类的对象的右值引用
* 返回值：  void
*/
void LogSys::output(LOG_LEVEL level, std::initializer_list<string>&& str)
{
    std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
    string outputText, tempLevel;

    switch (level)
    {
        case DEBUG:
        {
            tempLevel = "[DEBUG]";
            break;
        }
        case INFO:
        {
            tempLevel = "[INFO] ";
            break;
        }
        case ERROR:
        {
            tempLevel = "[ERROR]";
            break;
        }
        case WARN:
        {
            tempLevel = "[WARN] ";
            break;
        }
        default:
            break;
    }
    assert(tempLevel.size() > 0);

    lock.lock();
    string date;
    getDate(date, m_timeRecord);
    outputText = tempLevel + '[' + date + ' ' + m_timeRecord + "] ";

    for (auto i : str)
        outputText += i;
    
    if (date != m_dateRecord)
    {
        m_dateRecord = date;
        m_lineCount = 0;
        m_logPage = 1;
        m_fileName = m_dateRecord + "-1.log";
    }

    if (m_logIsAsync)
    {
        string* str = new string(outputText);
        m_blockQueue->addTask(str);
        return;
    }

    ++m_lineCount;
    int k = m_maxLineSize - m_lineCount;
    
    if (k < 0)
    {
        ++m_logPage;
        m_lineCount = 1;
        m_fileName = m_dateRecord + '-' + std::to_string(m_logPage) + ".log";
    }

    m_outputFile.open(m_fileName, std::ios_base::app);
    if (m_outputFile.is_open())
    {
        m_outputFile << outputText << std::endl;
        m_outputFile.close();
    }
}

/*
* 函数功能： 异步模式下日志输出的线程运行函数，向日志文件中写日志
* 输入参数： NULL
* 返回值：  void
*/
void LogSys::threadLoop()
{
    string* temp;
    while (true)
    {
        m_blockQueue->takeTask(temp);
        assert(temp != NULL);

        ++m_lineCount;
        int k = m_maxLineSize - m_lineCount;

        if (k < 0)
        {
            ++m_logPage;
            m_lineCount = 1;
            m_fileName = m_dateRecord + '-' + std::to_string(m_logPage) + ".log";
        }

        m_outputFile.open(m_fileName, std::ios_base::app);
        if (m_outputFile.is_open())
        {
            m_outputFile << *temp << std::endl;
            m_outputFile.close();
        }

        m_blockQueue->delTask();
        delete temp;
    }
}

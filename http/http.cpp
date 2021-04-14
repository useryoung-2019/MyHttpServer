#include "http.hpp"
typedef std::initializer_list<std::string> STR;

int Http::s_epollFd = -1;
int Http::s_count = 0;

/*
* 函数功能： 初始化Http对象的sockfd， sockaddr以及触发模式
* 输入参数： sockfd 通信的sockfd
*           addr 通信的sock地址
*           modeET sockfd在EPOLL句柄上的触发模式，ET为true，LT为false
* 返回值：  void
*/
void Http::initSocket(int sockfd, const sockaddr_in &addr, bool modeET)
{
    if (sockfd <= 0)
    {
        ERROR({"sockfd is bad!"});
        exit(-1);
    }

    m_sockfd = sockfd;
    m_addr = addr;
    m_modeET = modeET;
    ++s_count;

    epollAddfd(s_epollFd, m_sockfd, m_modeET);
    initHttp();
}

void Http::initHttp()
{
    memset(m_readBuf, '\0', READ_BUF_SIZE);
    m_readBufPara.readIndex = 0;
    m_readBufPara.checkIndex = 0;
    m_readBufPara.lineMark = 0;

    m_parseState = PARSE_REQUESTLINE;
    m_method = GET;

    m_requestPara.contentLen = 0;

    m_fileMmapAddr = NULL;

    memset(m_writeBuf, '\0', WRITE_BUF_SIZE);
    m_writeBufPara.iovecCount = 2;
    m_writeBufPara.writeIndex = 0;
    m_writeBufPara.bufSize = 0;
    m_writeBufPara.bufSendMark = 0;

    m_mysql = NULL;
}

/*
* 函数功能： 依据触发模式采取不同的读取对端发送的报文消息
* 输入参数： void
* 返回值：  true 成功读取
           false 读取长度超过限定长度，或者读取发生错误
*/
bool Http::readFromSocket()
{
    int readSize = 0;

    if (m_readBufPara.readIndex > READ_BUF_SIZE)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__),
                 "-> Reading buffer is full!"}));
        return false;
    }

    if (m_modeET)
    {
        while (true)
        {
            readSize = read(m_sockfd, m_readBuf + m_readBufPara.readIndex, READ_BUF_SIZE);

            if (readSize == -1)/*非堵塞模式下read的返回值分情况*/
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    break;
                }
                return false;
            }
            else if (readSize == 0)
            {
                return true;
            }

            m_readBufPara.readIndex += readSize;
        }
        DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> Get request! Size of request is ", 
                   std::to_string(m_readBufPara.readIndex)}));
        INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> \n",m_readBuf}));
        return true;
    }

    readSize = read(m_sockfd, m_readBuf + m_readBufPara.readIndex, READ_BUF_SIZE);

    if (readSize < 0)
    {
        return false;
    }

    m_readBufPara.readIndex += readSize;
    DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> Get request! Size of request is ", 
               std::to_string(m_readBufPara.readIndex)}));
    return true;
}

/*
* 函数功能： 由状态机返回分析状态决定写入响应报文的内容
* 输入参数： void
* 返回值：  void
*/
void Http::run()
{
    RESPOND_STATE stateRet = stateTransform();
    DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> stateRET ", std::to_string(stateRet)}));
    
    if (stateRet == NO_REQUEST)
    {
        epollModfd(s_epollFd, m_sockfd, EPOLLIN, m_modeET);
        initHttp();/*没有请求需要初始化参数，下次读写会发生无法预测的结果*/
        DEBUG(STR({__FILE__, ":", std::to_string(__LINE__),
                 "-> Mod Fd status to EPOLLIN beacuse of NO_REQUEST!"}));
        return;
    }

    bool ret = writeResponse(stateRet);

    if (ret == false)
    {
        if (m_sockfd != -1)
        {
            epollDelfd(s_epollFd, m_sockfd);
            ERROR(STR({__FILE__, ":", std::to_string(__LINE__), 
                        "-> Del Fd From epollfd because of BAD_REQUEST or other un_enum state!"}));
            m_sockfd = -1;
            --s_count;
            return;
        }
    }

    epollModfd(s_epollFd, m_sockfd, EPOLLOUT, m_modeET);/*完全写好响应消息时则应该修改触发状态为OUT*/
    DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), 
               "-> Mod Fd status To EPOLLOUT because of thread is ready to call WriteToSocket"}));
}

/*
* 函数功能： 分析整个请求报文
* 输入参数： void
* 返回值：  请求报文的状态
*/
Http::RESPOND_STATE Http::stateTransform()
{
    char *text = NULL;/*存储每一段请求行*/
    bool parseComplete = false;/*作为解析到消息体的标识*/
    RESPOND_STATE stateRet;

    while ((m_parseState == PARSE_CONTENT && !parseComplete) ||
           (parseLine() == LINE_OK))
    {
        text = [this]()-> char * { return m_readBuf + m_readBufPara.lineMark; }();
        DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> TEXT:" ,text}));
        m_readBufPara.lineMark = m_readBufPara.checkIndex;//每次更新一个新行就要记录下一行的截断位置

        switch (m_parseState)
        {
        case PARSE_REQUESTLINE:
        {
            stateRet = parseRequestLine(text);

            if (stateRet == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        }
        case PARSE_HEADER:
        {
            stateRet = parseHeader(text);

            if (stateRet == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            else if (stateRet == GET_REQUEST)
            {
                return handleRequest();
            }
            break;
        }
        case PARSE_CONTENT:
        {
            stateRet = parseContent(text);

            if (stateRet == GET_REQUEST)
            {
                return handleRequest();
            }
            parseComplete = true;
            break;
        }
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

/*
* 函数功能： 读取报文中直到出现\r\n两个特征字符即完
* 输入参数： void
* 返回值：  返回检查每一行的请求行的行状态
*/
Http::LINE_STATE Http::parseLine()
{
    char temp;
    for (; m_readBufPara.checkIndex < m_readBufPara.readIndex; ++m_readBufPara.checkIndex)
    {
        temp = m_readBuf[m_readBufPara.checkIndex];

        if (temp == '\r')
        {
            if ((m_readBufPara.checkIndex + 1) == m_readBufPara.readIndex)
            {
                return LINE_UNKNOW;
            }
            else if (m_readBuf[m_readBufPara.checkIndex + 1] == '\n')
            {
                m_readBuf[m_readBufPara.checkIndex++] = '\0';/*将\r转换为\0*/
                m_readBuf[m_readBufPara.checkIndex++] = '\0';/*将\r转换为\0,并且检查点递进到下一行*/
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if (m_readBufPara.checkIndex > 1 && m_readBuf[m_readBufPara.checkIndex - 1] == '\r')
            {
                m_readBuf[m_readBufPara.checkIndex - 1] = '\0';
                m_readBuf[m_readBufPara.checkIndex++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_UNKNOW;
}

/*
* 函数功能： 解析请求行,获得请求方法，url，version参数
* 输入参数： text 字符串
* 返回值：  返回解析请求行的请求状态
*/
Http::RESPOND_STATE Http::parseRequestLine(char *c)
{
    std::string text(c);

    int findRet = text.find_first_of(' ');
    if (findRet == -1)
    {
        DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> RequestLine is BAD"}));
        return BAD_REQUEST;
    }

    std::string tempStr(text, 0, findRet);
    if (tempStr == "GET")
    {
        m_method = GET;
    }
    else if (tempStr == "POST")
    {
        m_method = POST;
    }
    else
    {
        DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> RequestLine is BAD"}));
        return BAD_REQUEST;
    }

    INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> Method:", tempStr}));
    text.erase(0, findRet + 1);
    findRet = text.find_first_of(' ');
    if (findRet == -1)
    {
        DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> RequestLine is BAD"}));
        return BAD_REQUEST;
    }

    tempStr.clear();
    tempStr.assign(text.begin() + findRet + 1, text.end());
    if (tempStr == "HTTP/1.0" || tempStr == "HTTP/1.1")
    {
        m_requestPara.version = tempStr;
    }
    else
    {
        DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> RequestLine is BAD"}));
        return BAD_REQUEST;
    }
    INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> Version: ", m_requestPara.version}));

    text.erase(findRet);
    findRet = text.find_last_of('/');
    if (findRet == -1)
    {
        DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> RequestLine is BAD"}));
        return BAD_REQUEST;
    }

    text.erase(0, findRet);
    if (text.size() > 0)
    {
        if (text.size() == 1)
        {
            m_requestPara.url = "index.html";
        }
        else
        {
            m_requestPara.url.assign(text.begin() + 1, text.end());
        }
        INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> Url: ", m_requestPara.url}));
        m_parseState = PARSE_HEADER;
        return NO_REQUEST;
    }

    DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> RequestLine is BAD"}));
    return BAD_REQUEST;
}

/*
* 函数功能： 解析请求头部,获取host,conten-len
* 输入参数： text 字符串
* 返回值：  返回解析请求头部的请求状态
*/
Http::RESPOND_STATE Http::parseHeader(char *c)
{
    std::string text(c);
    DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> HeaderSize: ", std::to_string(text.size())}));
    int findRet = 0;
    if (text.size() == 0)//在parseLine中已经将\r\n全部转化为\0,因此无空行，所以剩下的就是消息体
    {
        if (m_requestPara.contentLen != 0)
        {
            m_parseState = PARSE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;/*消息体size为0则说明方法为GET*/
    }
    else if ((findRet = text.find("Host: ", 0)) != -1)
    {
        m_requestPara.host.assign(text.begin() + findRet + 6, text.end());
        INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> Host: ", m_requestPara.host}));
    }
    else if ((findRet = text.find("Content-Length: ", 0)) != -1)
    {
        text.erase(text.begin(), text.begin() + findRet + 16);
        char *end;
        m_requestPara.contentLen = static_cast<int>(std::strtol(text.c_str(), &end, 10));
        INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> Conten_Len: ", text}));
    }
    else if ((findRet = text.find("Connection: ", 0)) != -1)
    {
        text.erase(text.begin(), text.begin() + findRet + 12);
        m_requestPara.connection = text;
        INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> Connection: ", text}));
    }
    return NO_REQUEST;
}

/*
* 函数功能： 解析请求消息体
* 输入参数： text 字符串
* 返回值：  返回解析请求消息体的请求状态
*/
Http::RESPOND_STATE Http::parseContent(char *c)
{
    std::string text(c);

    if (text.size() == m_requestPara.contentLen)/*相同则直接获得消息体否则说明有问题*/
    {
        m_requestPara.requestPost = text;
        INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> Content: ", m_requestPara.requestPost}));
        m_readBufPara.checkIndex = m_readBufPara.readIndex;
        memset(m_readBuf, '\0', READ_BUF_SIZE);
        return GET_REQUEST;
    }

    m_readBufPara.checkIndex = m_readBufPara.readIndex;
    memset(m_readBuf, '\0', READ_BUF_SIZE);
    return NO_REQUEST;
}

/*
* 函数功能： 处理请求
* 输入参数： void
* 返回值：  返回处理请求后的状态
*/
Http::RESPOND_STATE Http::handleRequest()
{
    m_fileName = getcwd(NULL, 0);
    m_fileName += "/html/";/*Release Version*/

    //m_fileName = "/home/young/VSCODE/program/html/";debug version

    if (m_method == POST && m_requestPara.contentLen != 0)/*POST方法下的处理，直接请求资源*/
    {
        int findRet = m_requestPara.requestPost.find("submit=");
        if (findRet == 0)
        {
            m_requestPara.requestPost.erase(0, 7);
            if (m_requestPara.requestPost == "register")
            {
                m_fileName += "register.html";
            }
            else if (m_requestPara.requestPost == "load")
            {
                m_fileName += "load.html";
            }
            else if (m_requestPara.requestPost == "picture")
            {
                m_fileName += "picture.html";
            }
            else if (m_requestPara.requestPost == "audio")
            {
                m_fileName += "audio.html";
            }
            else
                return BAD_REQUEST;
        }
        else/*数据库账户注册或登录请求*/
        {
            findRet = m_requestPara.requestPost.find("User=");
            std::string temp(m_requestPara.requestPost.begin(), m_requestPara.requestPost.begin() + findRet);
            m_requestPara.requestPost.erase(0, findRet + 5);
            findRet = m_requestPara.requestPost.find("&password=");
            string name(m_requestPara.requestPost.begin(), m_requestPara.requestPost.begin() + findRet);
            string password(m_requestPara.requestPost.begin() + findRet + 10, m_requestPara.requestPost.end());

            bool ret = ConnectPool::getInstance()->getMysql(m_mysql);
            if (!ret)
            {
                WARN(STR({__FILE__, ":", std::to_string(__LINE__), "-> Cannot get mysql connect from ConnectPool"}));
                return INTERNAL_ERROR;
            }

            std::pair<std::string, std::string> account;
            ACCOUNT_STATE stateRet = checkAccount(m_mysql, name, account);
            if (stateRet == ACCOUNT_ERROR)
            {
                return INTERNAL_ERROR;
            }

            if (temp == "register")
            {
                if (stateRet == ACCOUNT_EXIST)/*注册账户已存在*/
                {
                    m_fileName += "Reregister.html";
                }
                else
                {
                    std::string insert("INSERT INTO webuser(name, password) VALUE(");
                    insert = insert + "'" + name + "','" + password + "')";
                    int ret = mysql_real_query(m_mysql, insert.c_str(), insert.size());

                    if (ret == 0)/*成功写入数据库*/
                    {
                        m_fileName += "load.html";
                    }
                    else
                    {
                        m_fileName += "Reregister.html";
                    }
                }
            }
            else if (temp == "load")
            {
                if (password == account.second)/*登陆成功*/
                {
                    m_fileName += "homepage.html";
                }
                else
                {
                    m_fileName += "Reload.html";
                }
            }
            else
            {
                ConnectPool::getInstance()->releaseMysql(m_mysql);
                return NO_REQUEST;
            }
            ConnectPool::getInstance()->releaseMysql(m_mysql);/*数据库连接释放*/
        }
    } 
    else if (m_method == GET && m_requestPara.contentLen == 0)/*GET请求*/
    {
        m_fileName += m_requestPara.url;
    }
    else
    {
        return BAD_REQUEST;
    }

    INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> RequestSource: ", m_fileName}));

    if (stat(m_fileName.c_str(), &m_fileStat) < 0)/*无该文件*/
    {
        return NO_RESOURCE;
    }
    
    if (S_ISDIR(m_fileStat.st_mode))/*请求资源是个文件夹*/
    {
        return BAD_REQUEST;
    }

    if (!(m_fileStat.st_mode & S_IROTH))/*无访问权限*/
    {
        return FORBIDDEN_REQUEST;
    }

    int fd = open(m_fileName.c_str(), O_RDONLY);
    m_fileMmapAddr = (char*)mmap(0, m_fileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    return FILE_REQUEST;
}

/*
* 函数功能： 接入数据库，获取账户的信息
* 输入参数： mysql 有效的mysql连接
*           name 账户名
* 输出参数： account 账户信息获取成功则将账户信息返回到account中，否则不写入account
* 返回值：  账户查询状态
*/
Http::ACCOUNT_STATE Http::checkAccount(MYSQL* mysql, const std::string& name,
                                       std::pair<std::string, std::string>& account)
{
    if (name.size() == 0)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__), "-> Get user account failure"}));
        return ACCOUNT_ERROR;
    }

    std::string select = "SELECT password FROM webuser WHERE name='" + name + "'";
    mysql_real_query(mysql, select.c_str(), select.size());
    MYSQL_RES* res = mysql_store_result(mysql);
    
    if (res != NULL)
    {
        unsigned num = mysql_num_rows(res);

        if (num == 0)
        {
            mysql_free_result(res);
            return ACCOUNT_NOT_EXIST;
        }
        else if(num == 1)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            string temp = row[0];
            account.first = name;
            account.second = temp;
            mysql_free_result(res);
            return ACCOUNT_EXIST;
        }
        else
        {
            ERROR(STR({__FILE__, ":", std::to_string(__LINE__), "-> The same name exist in database"}));
            mysql_free_result(res);
            return ACCOUNT_ERROR;
        }
    }
    return ACCOUNT_ERROR;
}

/*
* 函数功能： 向client发送信息
* 输入参数： void
* 返回值：  发送成功返回true，发送失败返回false
*/
bool Http::writeToSocket()
{
    int temp = 0;

    if (m_writeBufPara.bufSize == 0)
    {
        epollModfd(s_epollFd, m_sockfd, EPOLLIN, m_modeET);
        DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> Fd is Moded to EPOLLIN because of no-buf to write"}));
        initHttp();/*初始化很重要*/
        return true;
    }

    while(true)
    {
        temp = writev(m_sockfd, m_writeBufPara.writeIovec, m_writeBufPara.iovecCount);
        /*大文件一次性写不完，需要频繁修改写入的偏移量以确定正确的发送文件*/
        if (temp < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), 
                          "-> Fd is Mod to EPOLLOUT because writev-buffer is full and errno is EAGAIN"}));
                epollModfd(s_epollFd, m_sockfd, EPOLLOUT, m_modeET);
                return true;
            }
            else
            {
                releaseMmap();
                return false;
            }
        }

        m_writeBufPara.bufSendMark += temp;
        m_writeBufPara.bufSize -= temp;
        /*每次写都要重新修改写入的文件*/
        if (m_writeBufPara.bufSendMark >= m_writeBufPara.writeIovec[0].iov_len)
        {
            m_writeBufPara.writeIovec[0].iov_len = 0;
            m_writeBufPara.writeIovec[1].iov_base = m_fileMmapAddr + m_writeBufPara.bufSendMark - m_writeBufPara.writeIndex;
            m_writeBufPara.writeIovec[1].iov_len = m_writeBufPara.bufSize;
        }
        else
        {
            m_writeBufPara.writeIovec[0].iov_base = m_writeBuf + m_writeBufPara.bufSendMark;
            m_writeBufPara.writeIovec[0].iov_len = m_writeBufPara.writeIovec[0].iov_len - m_writeBufPara.bufSendMark;
        }

        if (m_writeBufPara.bufSize <= 0)
        {
            INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> ",m_fileName, " is already sent to client"}));
            releaseMmap();
            DEBUG(STR({__FILE__, ":", std::to_string(__LINE__), "-> Fd is Mod to EPOLLIN because of buf is sent completely!"}));
            epollModfd(s_epollFd, m_sockfd, EPOLLIN, m_modeET);
            initHttp();/*很重要*/
            return true;
        }
    }
}

/*
* 函数功能： 根据报文的最终解析状态组合响应报文
* 输入参数： ret 报文的解析状态
* 返回值：  组合成功返回true，否则返回false
*/
bool Http::writeResponse(RESPOND_STATE ret)
{
    switch (ret)
    {
        case INTERNAL_ERROR:
        {
            writeRespondLine(500, "Internal Server Error");
            writeRespondBlank();
            INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> \n",m_writeBuf}));
            break;
        }
        case BAD_REQUEST:
        {
            writeRespondLine(400, "Bad Request");
            writeRespondBlank();
            INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> \n",m_writeBuf}));
            break;
        }
        case FORBIDDEN_REQUEST:
        {
            writeRespondLine(403, "Forbidden");
            writeRespondBlank();
            INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> \n",m_writeBuf}));
            break;
        }
        case NO_RESOURCE:
        {
            writeRespondLine(404, "Not Found");
            writeRespondBlank();
            INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> \n",m_writeBuf}));
            break;
        }
        case FILE_REQUEST:
        {
            writeRespondLine(200, "OK");

            if (m_fileStat.st_size != 0)
            {
                writeRespondContentLen(m_fileStat.st_size);
                writeRespondContentType();
                //writeRespondConnection();
                writeRespondBlank();

                m_writeBufPara.writeIovec[0].iov_base = m_writeBuf;
                m_writeBufPara.writeIovec[0].iov_len = m_writeBufPara.writeIndex;/*该部分是响应报文的行与头部*/
                m_writeBufPara.writeIovec[1].iov_base = m_fileMmapAddr;
                m_writeBufPara.writeIovec[1].iov_len = m_fileStat.st_size;/*响应报文的消息体*/
                m_writeBufPara.bufSize = m_writeBufPara.writeIndex + m_fileStat.st_size;

                INFO(STR({__FILE__, ":", std::to_string(__LINE__), "-> \n",m_writeBuf}));
                return true;
            }
            else
            {
                writeRespondContent(0);
                writeRespondBlank();
                break;
            }
        }
        default:
            return false;
    }
    m_writeBufPara.iovecCount = 1;
    m_writeBufPara.writeIovec[0].iov_base = m_writeBuf;
    m_writeBufPara.writeIovec[0].iov_len = m_writeBufPara.writeIndex;
    m_writeBufPara.bufSize = m_writeBufPara.writeIndex;
    return true;
}

void Http::writeRespond(const char* format, ...)
{
    if (m_writeBufPara.writeIndex > WRITE_BUF_SIZE)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__), "-> Respnd Message'size is too large"}));
        return;
    }

    va_list arg;
    va_start(arg, format);

    int len = vsnprintf(m_writeBuf + m_writeBufPara.writeIndex, WRITE_BUF_SIZE - 
                        m_writeBufPara.writeIndex - 1, format, arg);
                        
    if (len >= WRITE_BUF_SIZE - m_writeBufPara.writeIndex - 1)
    {
        ERROR(STR({__FILE__, ":", std::to_string(__LINE__), "-> Respnd Message'size is too large"}));
        va_end(arg);
        return;
    }
    m_writeBufPara.writeIndex += len;
    va_end(arg);
}

void Http::writeRespondLine(int state, const char* stateDescription)
{
    writeRespond("%s %d %s\r\n", "HTTP/1.1", state, stateDescription);
}

void Http::writeRespondContentType()
{
    writeRespond("Content-Type: %s\r\n", "text/html; charset=UTF-8; image/x-icon; image/png; audio/*");
}

void Http::writeRespondContentLen(int len)
{
    writeRespond("Content-Length: %d\r\n", len);
}

void Http::writeRespondConnection()
{
    writeRespond("Connection: %s", m_requestPara.connection.c_str());
}

void Http::writeRespondBlank()
{
    writeRespond("%s", "\r\n");
}

void Http::writeRespondContent(const char* text)
{
    writeRespond("%s", text);
}

void Http::releaseMmap()
{
    if (m_fileMmapAddr!= NULL)
    {
        munmap(m_fileMmapAddr, m_fileStat.st_size);
        m_fileMmapAddr = NULL;
    }
}

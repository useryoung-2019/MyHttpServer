#include"Http.hpp"

int Http::g_epollfd = -1;

void Http::init(int sockfd, const sockaddr_in& addr, bool triggerET)
{
    if (sockfd <= 0)
    {
        ERROR(LOG({"Bad sockfd!"}));
        exit(-1);
    }

    m_sockfd = sockfd;
    m_addr = addr;
    m_triggerET = triggerET;
    
    epollAddfd(g_epollfd, m_sockfd, triggerET);
    init();
    
    std::cout << "初始化http\n";
}

void Http::init()
{
    m_parseState = PARSE_REQUEST_LINE;
    m_method = GET;
    m_accountState = ACCOUNT_NOTEXIST;
    m_fileMmap = nullptr;
    memset(m_readBuf, '\0', READBUF_SIZE);
    memset(m_writeBuf, '\0', WRITEBUF_SIZE);
    m_readPara.clear();
    m_writePara.clear();
    m_httpMsg.clear();
}

bool Http::Read()
{
    int size = 0;
    if (m_readPara.readIndex >= READBUF_SIZE)
    {
        ERROR(LOG({__FILE__, ": Reading buffer is full!"}));
        return false;
    }

    if (m_triggerET)
    {
        while (true)
        {
            size = read(m_sockfd, m_readBuf + m_readPara.readIndex, READBUF_SIZE);

            if (size == -1)
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                    break;
                return false;
            }
            else if (size == 0)
                return true;
            
            m_readPara.readIndex += size;
        }
        DEBUG(LOG({__FILE__, ": ", std::to_string(m_readPara.readIndex)}));
        INFO(LOG({__FILE__, ": \n", m_readBuf}));
        return true;
    }
    
    size = read(m_sockfd, m_readBuf, READBUF_SIZE);
    if (size < 0)
        return false;

    m_readPara.readIndex += size;
    INFO(LOG({__FILE__, ": \n", m_readBuf}));
    return true;
}

Http::LINE_STATE Http::parseLine()
{
    char temp;
    for (; m_readPara.checkIndex < m_readPara.readIndex; ++m_readPara.checkIndex)
    {
        temp = m_readBuf[m_readPara.checkIndex];

        if (temp == '\r')
        {
            if ((m_readPara.checkIndex + 1) == m_readPara.readIndex)
                return LINE_UNKNOWN;
            else if (m_readBuf[m_readPara.checkIndex + 1] == '\n')
            {
                m_readBuf[m_readPara.checkIndex++] = '\0';
                m_readBuf[m_readPara.checkIndex++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if (m_readPara.checkIndex > 1 && m_readBuf[m_readPara.checkIndex - 1] == '\r')
            {
                m_readBuf[m_readPara.checkIndex - 1] = '\0';
                m_readBuf[m_readPara.checkIndex++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }

    return LINE_UNKNOWN;
}

Http::RESPOND_STATE Http::stateTransform()
{
    while (m_parseState == PARSE_PREV_CONTENT || m_parseState == PARSE_CONTENT || parseLine() == LINE_OK)
    {
        auto text = [&]()-> char* {
            return m_readBuf + m_readPara.line;}();
        
        m_readPara.line = m_readPara.checkIndex;

        switch (m_parseState)
        {
            case PARSE_REQUEST_LINE:
            {
                auto ret = parseRequestLine(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case PARSE_HEADER:
            {
                auto ret = parseHeader(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                else if (ret == GET_REQUEST)
                    return handleRequest();
                break;
            }
            case PARSE_PREV_CONTENT:
            {
                m_parseState = PARSE_CONTENT;
                break;
            }
            case PARSE_CONTENT:
            {
                auto ret = parseContent(text);
                if (ret == GET_REQUEST)
                    return handleRequest();
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

Http::RESPOND_STATE Http::parseRequestLine(string text)
{
    DEBUG(LOG({__FILE__, ": ", text}));
    string str;
    if (text[0] == 'G')
    {
        str = text.substr(0, 3);
        if (str == "GET" && text[3] == ' ')
            m_method = GET;
        else
            return BAD_REQUEST;
    }
    else if (text[0] == 'P')
    {
        str = text.substr(0, 4);
        if (str == "POST" && text[4] == ' ')
            m_method = POST;
        else
            return BAD_REQUEST;
    }
    else
        return BAD_REQUEST;
    
    INFO(LOG({__FILE__, ": PARSE method ", str}));
    text.erase(0, str.length() + 1);
    str.clear();

    auto ret = text.find(' ');
    if (ret >= text.size())
        return BAD_REQUEST;
    
    str = text.substr(ret + 1, 8);
    if (str == "HTTP/1.0" || str == "HTTP/1.1")
    {
        m_httpMsg.Version = str;
        INFO(LOG({__FILE__, ": PARSE version ", str}));
    }
    else
        return BAD_REQUEST;
    
    text.erase(ret, 9);
    str.clear();

    ret = text.rfind('/');
    if (ret >= text.length())
        return BAD_REQUEST;
    text.erase(0, ret);
    
    if (text.length() > 0)
    {
        if (m_method == GET)
        {
            if (text.length() == 1)
                m_httpMsg.URL = "index.html";
            else
                m_httpMsg.URL.assign(text.begin() + 1, text.end());
            
            INFO(LOG({__FILE__, ":PARSE URL ", m_httpMsg.URL}));
        }
        m_parseState = PARSE_HEADER;
        return NO_REQUEST;
    }
    else
        return BAD_REQUEST;
}

Http::RESPOND_STATE Http::parseHeader(string text)
{
    INFO(LOG({__FILE__, ": ", text}));

    if (text.length() == 0)
    {
        if (m_method == GET)
            return GET_REQUEST;
        else
        {
            m_parseState = PARSE_PREV_CONTENT;
            return NO_REQUEST;
        }
    }
    
    string str = text.substr(0, 4);
    if (str == "Host" && text[4] == ':')
    {
        m_httpMsg.Host.assign(text.begin() + 6, text.end());
        INFO(LOG({__FILE__, ":PARSE Host ", m_httpMsg.Host}));
        return NO_REQUEST;
    }

    str = text.substr(0, 14);
    if (str == "Content-Length" && text[14] == ':')
    {
        str.assign(text.begin() + 16, text.end());
        char* end;
        m_httpMsg.ContentLen = static_cast<int>(std::strtol(str.c_str(), &end, 0));
        INFO(LOG({__FILE__, ":PARSE Content-Length ", str}));
        return NO_REQUEST;
    }
    
    return NO_REQUEST;
}

Http::RESPOND_STATE Http::parseContent(string text)
{
    if (text.size() == m_httpMsg.ContentLen)
    {
        m_httpMsg.Content = text;
        INFO(LOG({__FILE__, ":PARSE Content ", text}));
        m_readPara.checkIndex = m_readPara.readIndex;
        return GET_REQUEST;
    }

    m_readPara.checkIndex = m_readPara.readIndex;
    memset(m_readBuf, '\0', READBUF_SIZE);
    return NO_REQUEST;
}

Http::RESPOND_STATE Http::handleRequest()
{
    m_requestFile = getcwd(NULL, 0);
    m_requestFile += "/html/";

    if (m_method == GET && m_httpMsg.ContentLen == 0)
        m_requestFile += m_httpMsg.URL;
    else if (m_method == POST && m_httpMsg.ContentLen != 0)
    {
        int ret = m_httpMsg.Content.find("submit=");
        if (ret == 0)
        {
            m_httpMsg.Content.erase(0, 7);
            m_requestFile += m_httpMsg.Content + ".html";
        }
        else
        {
            ret = m_httpMsg.Content.find("User=");
            string temp(m_httpMsg.Content.begin(), m_httpMsg.Content.begin() + ret);
            m_httpMsg.Content.erase(0, ret + 5);
            ret = m_httpMsg.Content.find("&password=");
            string name(m_httpMsg.Content.begin(), m_httpMsg.Content.begin()+ret);
            string password(m_httpMsg.Content.begin() + ret + 10, m_httpMsg.Content.end());

            pair<string, string> account;
            m_accountState = checkAccount(name, password, account);

            if (temp == "register")
                registerSQL(name, password);
            else if (temp == "load")
            {
                if (password == account.second)
                    m_requestFile += "homepage.html";
                else
                    m_requestFile += "Reload.html";
            }
            else
                return NO_REQUEST;
        }
    }
    else
        return BAD_REQUEST;
    
    INFO(LOG({__FILE__, ":FILE ", m_requestFile}));

    if (stat(m_requestFile.c_str(), &m_fileStat) < 0)
        return NO_RESOURCE;
    
    if (S_ISDIR(m_fileStat.st_mode))
        return BAD_REQUEST;
    
    if (!(m_fileStat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;
    
    int fd = open(m_requestFile.c_str(), O_RDONLY);
    m_fileMmap = static_cast<char*>(mmap(0, m_fileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    close(fd);

    return FILE_REQUEST;
}

Http::SQL_RES Http::checkAccount(string& name, string& passwd, pair<string, string>& account)
{
    shared_ptr<AbstractFactory> redisFac(new FactoryRedis);
    shared_ptr<AbstractFactory> mysqlFac(new FactoryMysql);

    if (!redisFac->createSQL()->selectFromSQL(name, account))
    {
        if (!mysqlFac->createSQL()->selectFromSQL(name, account))
            return ACCOUNT_NOTEXIST;
    }
    return ACCOUNT_EXIST;
}

void Http::registerSQL(string& name, string& password)
{
    if (m_accountState == ACCOUNT_EXIST)
        m_requestFile += "Reregister.html";
    else
    {
        shared_ptr<AbstractFactory> redisFac(new FactoryRedis);
        string query("HSET webuser ");
        query = query + name + " " + password;
        if (!redisFac->createSQL()->IDUIntoSQL(query))
        {
            WARN(LOG({__FILE__, ": Cannot insert into Redis"}));
            m_requestFile += "Reregister.html";
            return;
        }

        shared_ptr<AbstractFactory> mysqlFac(new FactoryMysql);
        query = "insert into webuser(name, passwd) values('" + name +"','" + password + "')";
        if (!mysqlFac->createSQL()->IDUIntoSQL(query))
        {
            WARN(LOG({__FILE__, ": Cannot insert into Redis"}));
            m_requestFile += "Reregister.html";
            return;
        }

        m_requestFile += "load.html";
    }
}

void Http::run()
{
    auto ret = stateTransform();
    DEBUG(LOG({__FILE__, ": ParseHttpMsg result ", std::to_string(ret)}));

    if (ret == NO_REQUEST)
    {
        epollModfd(g_epollfd, m_sockfd, EPOLLIN, m_triggerET);
        init();
        return;
    }

    bool writeRes = writeResponse(ret);

    std::cout << "wirteRES:  " << writeRes << std::endl;
    
    if (!writeRes)
    {
        if (m_sockfd != -1)
        {
            epollDelfd(g_epollfd, m_sockfd);
            m_sockfd = -1;
            return;
        }
    }

    epollModfd(g_epollfd, m_sockfd, EPOLLOUT, m_triggerET);
    DEBUG(LOG({__FILE__, ": ready to send to client!"}));
}

bool Http::writeResponse(RESPOND_STATE state)
{
    switch (state)
    {
        case INTERNAL_ERROR:
        {
            writeResponseLine(500, "Internal Server Error");
            writeBlank();
            break;
        }
        case BAD_REQUEST:
        {
            writeResponseLine(400, "Bad Request");
            writeBlank();
            break;
        }
        case FORBIDDEN_REQUEST:
        {
            writeResponseLine(403, "Forbidden");
            writeBlank();
            break;
        }
        case NO_RESOURCE:
        {
            writeResponseLine(404, "Not Found");
            writeBlank();
            break;
        }
        case FILE_REQUEST:
        {
            writeResponseLine(200, "OK");

            std::cout << "写头部\n" << m_fileStat.st_size << std::endl;

            if (m_fileStat.st_size != 0)
            {
                writeContentType();
                writeContentLen(m_fileStat.st_size);
                writeBlank();

                m_writePara.iov[0].iov_base = m_writeBuf;
                m_writePara.iov[0].iov_len = m_writePara.writeIndex;
                m_writePara.iov[1].iov_base = m_fileMmap;
                m_writePara.iov[1].iov_len = m_fileStat.st_size;
                m_writePara.bufSize = m_writePara.writeIndex + m_fileStat.st_size;

                INFO(LOG({__FILE__, ": Response\n", m_writeBuf}));
                return true;
            }
            else
            {
                writeContentLen(0);
                writeBlank();
                break;
            }
        }
        default:
            return false;
    }

    m_writePara.iocNum = 1;
    m_writePara.iov[0].iov_base = m_writeBuf;
    m_writePara.iov[0].iov_len = m_writePara.writeIndex;
    m_writePara.bufSize = m_writePara.writeIndex;
    return true;
}

void Http::writeHttpResponse(const char* format, ...)
{
    if (m_writePara.writeIndex >= WRITEBUF_SIZE)
    {
        ERROR(LOG({__FILE__, ": Http Response'size is too long"}));
        return;
    }

    va_list arg;
    va_start(arg, format);

    int len = vsnprintf(m_writeBuf + m_writePara.writeIndex, WRITEBUF_SIZE - m_writePara.writeIndex - 1, format, arg);

    if (len >= (WRITEBUF_SIZE - m_writePara.writeIndex - 1))
    {
        ERROR(LOG({__FILE__, ": Http Response'size is too long"}));
        va_end(arg);
        return;
    }

    m_writePara.writeIndex += len;

    va_end(arg);
}

void Http::writeResponseLine(int state, const char* description)
{
    writeHttpResponse("%s %d %s\r\n", "HTTP/1.1", state, description);
}

void Http::writeContentType()
{
    writeHttpResponse("Content-Type: %s\r\n", "text/html; charset=UTF-8; image/x-icon; image/png; audio/*");
}

void Http::writeContentLen(int len)
{
    writeHttpResponse("Content-Length: %d\r\n", len);
}

void Http::writeBlank()
{
    writeHttpResponse("%s", "\r\n");
}

void Http::writeContent(const char* text)
{
    writeHttpResponse("%s", text);
}

void Http::releaseMmap()
{
    if (m_fileMmap != nullptr)
    {
        munmap(m_fileMmap, m_fileStat.st_size);
        m_fileMmap = nullptr;
    }
}

bool Http::Write()
{
    int index = 0;

    if (m_writePara.bufSize == 0)
    {
        epollModfd(g_epollfd, m_sockfd, EPOLLIN, m_triggerET);
        init();
        return true;
    }

    while (true)
    {
        index = writev(m_sockfd, m_writePara.iov, m_writePara.iocNum);
        if (index < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                epollModfd(g_epollfd, m_sockfd, EPOLLOUT, m_triggerET);
                return true;
            }
            else
            {
                releaseMmap();
                return false;
            }
        }

        m_writePara.sendMark += index;
        m_writePara.bufSize -= index;

        if (m_writePara.bufSize <= 0)
        {
            INFO(LOG({__FILE__, ": Already send to client"}));
            releaseMmap();
            epollModfd(g_epollfd, m_sockfd, EPOLLIN, m_triggerET);
            init();
            return true;
        }

        if (m_writePara.sendMark >= m_writePara.iov[0].iov_len)
        {
            m_writePara.iov[0].iov_len = 0;
            m_writePara.iov[1].iov_base = m_fileMmap + m_writePara.sendMark - m_writePara.writeIndex;
            m_writePara.iov[1].iov_len = m_writePara.bufSize;
        }
        else
        {
            m_writePara.iov[0].iov_base = m_writeBuf + m_writePara.sendMark;
            m_writePara.iov[0].iov_len -= m_writePara.sendMark;
        }
    }
}
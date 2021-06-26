#ifndef HTTP_H
#define HTTP_H

#include<netinet/in.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<sys/uio.h>

#include"../log/Log.hpp"
#include"../SQL/ConnectPool.hpp"
#include"func.hpp"

struct ReadBuf
{
    int readIndex;
    int checkIndex;
    int line;

    ReadBuf():readIndex(0), checkIndex(0), line(0) {}
    
    void clear() 
    {
        readIndex = 0;
        checkIndex = 0;
        line = 0;
    }
};

struct HttpMsg
{
    string URL;
    string Host;
    string Version;
    string Content;
    size_t ContentLen;

    HttpMsg():ContentLen(0) {}
    
    void clear()
    {
        URL.clear();
        Host.clear();
        Version.clear();
        Content.clear();
        ContentLen = 0;
    }
};

struct WriteBuf
{
    struct iovec iov[2];
    size_t iocNum;
    int writeIndex;
    int bufSize;
    int sendMark;

    WriteBuf():iocNum(2), writeIndex(0), bufSize(0), sendMark(0){}

    void clear()
    {
        iocNum = 2;
        writeIndex = 0;
        bufSize = 0;
        sendMark = 0;
    }
};


class Http
{
public:
    enum METHOD {
        GET = 0,
        POST,
    };

    enum PARSE_STATE {
        PARSE_REQUEST_LINE = 0,
        PARSE_HEADER,
        PARSE_PREV_CONTENT,
        PARSE_CONTENT,
    };

    enum LINE_STATE {
        LINE_OK = 0,
        LINE_BAD,
        LINE_UNKNOWN,
    };

    enum RESPOND_STATE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
    };

    enum SQL_RES {
        ACCOUNT_EXIST = 0,
        ACCOUNT_NOTEXIST,
    };

    static const unsigned READBUF_SIZE = 1500;
    static const unsigned WRITEBUF_SIZE = 1500;

public:
    Http():m_writePara(), m_readPara(), m_httpMsg(){}
    ~Http() = default;

public:
    static int g_epollfd;

public:
    void init(int sockfd, const sockaddr_in& addr, bool triggerET);
    bool Read();
    bool Write();
    void run();

private:
    void init();
    RESPOND_STATE stateTransform();
    LINE_STATE parseLine();
    RESPOND_STATE parseRequestLine(string text);
    RESPOND_STATE parseHeader(string text);
    RESPOND_STATE parseContent(string text);
    RESPOND_STATE handleRequest();

    SQL_RES checkAccount(string& name, string& passwd, pair<string, string>& account);
    void registerSQL(string& name, string& password);
    /*
    SQL
    */
    bool writeResponse(RESPOND_STATE state);
    void writeHttpResponse(const char* format, ...);
    void writeResponseLine(int state, const char* description);
    void writeContentType();
    void writeContentLen(int len);
    void writeBlank();
    void writeContent(const char* text);

    void releaseMmap();


private:
    bool m_triggerET;
    int m_sockfd;
    sockaddr_in m_addr;

    char m_readBuf[READBUF_SIZE];
    struct ReadBuf m_readPara;

    PARSE_STATE m_parseState;
    METHOD m_method;
    struct HttpMsg m_httpMsg;

    SQL_RES m_accountState;

    string m_requestFile;
    struct stat m_fileStat;
    char* m_fileMmap;

    char m_writeBuf[WRITEBUF_SIZE];
    struct WriteBuf m_writePara;

};








#endif
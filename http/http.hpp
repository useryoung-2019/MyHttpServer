/********************************************************************************
作者： HAPPY YOUNG
日期： 2021-04-07
版本： v1.0
描述： 面向Http报文的类，仅限于处理POST/GET的简易请求，能够设置fd的ET/LT两种触发模式
使用： Http对象必须在使用前需要调用初始化函数，readFromSocket读取数据，run解析，
      writeToSocket写数据
********************************************************************************/

#ifndef HTTP_HPP_H
#define HTTP_HPP_H

#include<netinet/in.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<sys/uio.h>
#include<cstring>
#include<cstdarg>

#include"../connectpool/connectpool.hpp"
#include"../log/log_sys.hpp"
#include"func.hpp"

/*存储读取http请求报文的相关参数*/
struct ReadBufPara
{
    int readIndex;/*存储读取请求报文的最后位置*/
    int checkIndex;/*分析请求报文的位置*/
    int lineMark;/*记录请求报文的行的位置*/
};

/*存储分析请求报文的相关参数*/
struct RequestPara
{
    std::string url;/*url*/
    std::string host;/*host*/
    std::string version;/*version*/
    std::string connection;/*connection*/
    int contentLen;/*content-length*/
    std::string requestPost;/*请求消息体*/
};

/*存储写响应报文的相关参数*/
struct WriteBufPara
{
    struct iovec writeIovec[2];/*iovec结构体*/
    int iovecCount;/*iovec结构体的个数*/
    int writeIndex;/*记录写响应报文的响应行/头的最后位置*/
    int bufSize;/*需要发送的buf总长度*/
    int bufSendMark;/*已发送的buf长度位置offset*/
};

class Http
{
public:
    /*http报文请求行的请求头部的两个方法*/
    enum METHOD { 
                  GET = 0,/*GET请求的资源在请求行的url部分*/ 
                  POST/*POST请求的资源在请求内容中*/
                };

    /*主状态机开始解析当前字符串，分别对应http报文的三个部分*/
    enum PARSE_STATE { 
                       PARSE_REQUESTLINE = 0, /*请求行*/
                       PARSE_HEADER, /*请求头部*/
                       PARSE_CONTENT /*请求内容*/
                     };

    /*从状态机逐段分析，每段均有三个状态*/    
    enum LINE_STATE { 
                      LINE_OK = 0,/*该段字符串没有问题，符合要求*/ 
                      LINE_BAD, /*字符串有问题，没有继续分析下去的必要*/
                      LINE_UNKNOW/*字符串还没有完全分析，需要进一步读取分析*/
                    };

    /*通过http报文的分析，采取不同的手段回应*/
    enum RESPOND_STATE { 
                         NO_REQUEST = 0,/*分析后没有任何请求*/ 
                         GET_REQUEST, /*分析后得到请求*/
                         BAD_REQUEST, /*读取的报文存在错误*/
                         NO_RESOURCE,/*服务器不存在该请求资源*/
                         FORBIDDEN_REQUEST, /*服务器对请求资源没有权限*/
                         FILE_REQUEST, /*服务器得到该请求，并找到请求文件*/
                         INTERNAL_ERROR/*服务器内部错误*/
                        };
    /*数据库调用后的返回结果*/
    enum ACCOUNT_STATE{
                        ACCOUNT_EXIST = 0,/*账户已存在1个*/
                        ACCOUNT_NOT_EXIST,/*账户不存在*/
                        ACCOUNT_ERROR/*账户查询出现错误*/
                      };

    static const unsigned READ_BUF_SIZE = 1500;/*readBuf最大的读取长度*/
    static const unsigned WRITE_BUF_SIZE = 1500;/*writeBuf最大的可写长度*/

public:
    Http(){}
    ~Http(){}

public:
    static int s_epollFd;/*主进程创建的epoll句柄*/
    static int s_count;/*总共的活跃连接数量*/

private:
    bool m_modeET;/*sockfd采取的触发模式，ET/LT*/
    int m_sockfd;/*Http对象的通信fd*/
    sockaddr_in m_addr;/*Http对象的socket地址*/

    char m_readBuf[READ_BUF_SIZE];/*读取消息的存储体*/
    struct ReadBufPara m_readBufPara;

    PARSE_STATE m_parseState;/*分析报文的状态*/
    METHOD m_method;/*请求方法*/

    struct RequestPara m_requestPara;

    std::string m_fileName;/*请求资源的文件名*/
    struct stat m_fileStat;/*请求资源的文件信息*/
    char* m_fileMmapAddr;/*映射请求资源文件的地址*/

    char m_writeBuf[WRITE_BUF_SIZE];/*响应报文的存储体*/
    struct WriteBufPara m_writeBufPara;

    MYSQL* m_mysql;/*Http对象请求数据库连接*/

public:
    /*
    * 函数功能： Http对象初始化相关参数
    * 输入参数： sockfd 通信的sockfd
    *           addr 通信的sock地址
    *           modeET sockfd在EPOLL句柄上的触发模式
    * 返回值：  void
    */
    void initSocket(int sockfd, const sockaddr_in& addr, bool modeET);
    
    /*
    * 函数功能： 读取client发送的信息
    * 输入参数： void
    * 返回值：  成功读取返回true， 读取失败返回false
    */
    bool readFromSocket();
    
    /*
    * 函数功能： 向client发送信息
    * 输入参数： void
    * 返回值：  发送成功返回true，发送失败返回false
    */
    bool writeToSocket();
    
    /*
    * 函数功能： 分析请求报文
    * 输入参数： void
    * 返回值：  void
    */
    void run();

private:
    /*
    * 函数功能： 初始化对象的其余参数
    * 输入参数： void
    * 返回值：  void
    */
    void initHttp();

    /*
    * 函数功能： 主状态机
    * 输入参数： void
    * 返回值：  返回分析请求报文的状态
    */
    RESPOND_STATE stateTransform();

    /*
    * 函数功能： 解析请求报文的换行位置
    * 输入参数： void
    * 返回值：  返回每一行的请求行的行状态
    */
    LINE_STATE parseLine();

    /*
    * 函数功能： 解析请求行
    * 输入参数： text 字符串
    * 返回值：  返回解析请求行的请求状态
    */
    RESPOND_STATE parseRequestLine(char* text);

    /*
    * 函数功能： 解析请求头部
    * 输入参数： text 字符串
    * 返回值：  返回解析请求头部的请求状态
    */
    RESPOND_STATE parseHeader(char* text);

    /*
    * 函数功能： 解析请求消息体
    * 输入参数： text 字符串
    * 返回值：  返回解析请求消息体的请求状态
    */
    RESPOND_STATE parseContent(char* text);

    /*
    * 函数功能： 处理请求
    * 输入参数： void
    * 返回值：  返回处理请求后的状态
    */
    RESPOND_STATE handleRequest();

    /*
    * 函数功能： 接入数据库，获取账户的信息
    * 输入参数： mysql mysql连接
    *           name 账户名
    * 输出参数： account 将账户信息返回到account中
    * 返回值：  账户查询状态
    */
    ACCOUNT_STATE checkAccount(MYSQL* mysql, const std::string& name,
                                std::pair<std::string, std::string>& account);

    /*
    * 函数功能： 根据报文的最终解析状态组合响应报文
    * 输入参数： ret 报文的解析状态
    * 返回值：  组合成功返回true，否则返回false
    */
    bool writeResponse(RESPOND_STATE ret);

    /*
    * 函数功能： 组合响应报文的响应行与响应头部
    * 输入参数： format ,...
    * 返回值：  void
    */
    void writeRespond(const char* format, ...);
    
    /*
    * 函数功能： 写响应头部
    * 输入参数： state 状态码
    *          stateDescroption 状态描述
    * 返回值：  void
    */
    void writeRespondLine(int state, const char* stateDescription);
    
    /*
    * 函数功能： 写响应内容类型
    * 输入参数： void
    * 返回值：  void
    */
    void writeRespondContentType();

    /*
    * 函数功能： 写连接状态
    * 输入参数： void
    * 返回值：  void
    */
    void writeRespondConnection();

    /*
    * 函数功能： 写响应内容长度
    * 输入参数： len 响应消息体的长度
    * 返回值：  void
    */
    void writeRespondContentLen(int len);

    /*
    * 函数功能： 写消息体上部的空行
    * 输入参数： void
    * 返回值：  void
    */
    void writeRespondBlank();

    /*
    * 函数功能： 写响应消息体
    * 输入参数： text 请求资源的映射地址
    * 返回值：  void
    */
    void writeRespondContent(const char* text);

    /*
    * 函数功能： 释放映射位置
    * 输入参数： void
    * 返回值：  void
    */
    void releaseMmap();
};

#endif
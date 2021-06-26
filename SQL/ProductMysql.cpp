#include"ProductMysql.hpp"

bool ProductMysql::selectFromSQL(string& name, pair<string, string>& ret)
{
    MYSQL* mysql = nullptr;
    string str("select name, passwd from webuser where name = ");
    str = str + "'" + name + "'";
    if (MysqlConnectPool::getInstance()->getMYSQL(&mysql))
    {
        if (mysql_query(mysql, str.c_str()) != 0)
        {
            MysqlConnectPool::getInstance()->freeMYSQL(&mysql);
            mysql = nullptr;
            return false;
        }
            
        
        MYSQL_RES* res = mysql_store_result(mysql);
        if (mysql_num_rows(res) != 1)
        {
            MysqlConnectPool::getInstance()->freeMYSQL(&mysql);
            mysql_free_result(res);
            mysql = nullptr;
            return false;
        }
            

        MYSQL_ROW row = mysql_fetch_row(res);
        ret.first = row[0];
        ret.second = row[1];
        mysql_free_result(res);
        MysqlConnectPool::getInstance()->freeMYSQL(&mysql);
        mysql = nullptr;
        return true;
    }

    return false;
}

bool ProductMysql::selectFromSQL(string& str, vector<pair<string, string>>& ret)
{
    MYSQL* mysql = nullptr;
    ret.clear();
    if (MysqlConnectPool::getInstance()->getMYSQL(&mysql))
    {
        if (mysql_query(mysql, str.c_str()) != 0)
        {
            MysqlConnectPool::getInstance()->freeMYSQL(&mysql);
            mysql = nullptr;
            return false;
        }
        
        
        MYSQL_RES* res = mysql_store_result(mysql);
        ret.reserve(mysql_num_rows(res));
        MYSQL_ROW row;
        
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            string fir = row[0];
            string sec = row[1];
            ret.emplace_back(fir, sec);
        }

        mysql_free_result(res);
        MysqlConnectPool::getInstance()->freeMYSQL(&mysql);
        mysql = nullptr;
        return true;
    }

    return false;
}

bool ProductMysql::IDUIntoSQL(string& str)
{
    MYSQL* mysql = nullptr;
    
    if (MysqlConnectPool::getInstance()->getMYSQL(&mysql))
    {
        auto ret = mysql_query(mysql, str.c_str());
        MysqlConnectPool::getInstance()->freeMYSQL(&mysql);
        
        if (ret != 0)
        {
            mysql = nullptr;
            return false;
        }

        return true;
    }

    return false;
}
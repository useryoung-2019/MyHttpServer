#ifndef ABSTRACTPRODUCTSQL_H
#define ABSTRACTPRODUCTSQL_H

#include<string>
#include<vector>
using std::vector;
using std::string;
using std::pair;

class AbstractProductSQL
{
public:
    virtual bool selectFromSQL(string& str, pair<string, string>& ret) = 0;
    virtual bool selectFromSQL(string& str, vector<pair<string, string>>& ret) = 0;
    virtual bool IDUIntoSQL(string& str) = 0;
};

#endif
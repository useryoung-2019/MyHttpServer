#ifndef FACTORYMYSQL_H
#define FACTORYMYSQL_H

#include"AbstractFactory.hpp"
#include"ProductMysql.hpp"

class FactoryMysql:public AbstractFactory
{
public:
    shared_ptr<AbstractProductSQL> createSQL()
    {
        return shared_ptr<AbstractProductSQL>(new ProductMysql);
    }
};

#endif
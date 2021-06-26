#ifndef FACTORYREDIS_H
#define FACTORYREDIS_H

#include"AbstractFactory.hpp"
#include"ProductRedis.hpp"

class FactoryRedis:public AbstractFactory
{
public:
    shared_ptr<AbstractProductSQL> createSQL()
    {
        return shared_ptr<AbstractProductSQL>(new ProductRedis);
    }
};

#endif
#ifndef ABSTRACTFACTORY_H
#define ABSTRACTFACTORY_H

#include"AbstractProductSQL.hpp"
#include<memory>
using std::shared_ptr;

class AbstractFactory
{
public:
    virtual shared_ptr<AbstractProductSQL> createSQL() = 0;
};

#endif
server: main.cpp server.cpp http/Http.cpp http/func.cpp log/Log.cpp log/LogQueue.hpp ThreadPool/ThreadPool.hpp timer/Timer.cpp timer/TimerNode.cpp SQL/ConnectPool.hpp SQL/AbstractFactory.hpp SQL/FactoryMysql.hpp SQL/FactoryRedis.hpp SQL/AbstractProductSQL.hpp SQL/ProductMysql.cpp SQL/ProductRedis.cpp SQL/MysqlConnectPool.cpp SQL/RedisConnectPool.cpp
	g++ main.cpp server.cpp http/Http.cpp http/func.cpp log/Log.cpp log/LogQueue.hpp ThreadPool/ThreadPool.hpp timer/Timer.cpp timer/TimerNode.cpp SQL/ConnectPool.hpp SQL/AbstractFactory.hpp SQL/FactoryMysql.hpp SQL/FactoryRedis.hpp SQL/AbstractProductSQL.hpp SQL/ProductMysql.cpp SQL/ProductRedis.cpp SQL/MysqlConnectPool.cpp SQL/RedisConnectPool.cpp -g -o server -std=c++11 -lhiredis -lmysqlclient -pthread

.PHONY:clean
clean:
	-rm server
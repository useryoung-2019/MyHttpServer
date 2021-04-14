server.out: main.cpp http/http.hpp http/http.cpp http/func.hpp http/func.cpp connectpool/connectpool.hpp connectpool/connectpool.cpp threadpool/threadpool.hpp log/log_sys.hpp log/log_sys.cpp log/block_queue.hpp timer/timer_node.hpp timer/timer.hpp timer/timer.cpp
	g++ main.cpp http/http.hpp http/http.cpp http/func.hpp http/func.cpp connectpool/connectpool.hpp connectpool/connectpool.cpp threadpool/threadpool.hpp log/log_sys.hpp log/log_sys.cpp log/block_queue.hpp timer/timer_node.hpp timer/timer.hpp timer/timer.cpp -o server.out -pthread -lmysqlclient -g

.PHONY:clean
clean:
	-rm server.out
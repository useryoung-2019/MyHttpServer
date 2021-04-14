test.out:test.cpp ../http.hpp ../http.cpp ../func.hpp ../func.cpp ../../connectpool/connectpool.hpp ../../connectpool/connectpool.cpp ../../log/block_queue.hpp ../../log/log_sys.hpp ../../log/log_sys.cpp ../../threadpool/threadpool.hpp
	g++ test.cpp ../http.hpp ../http.cpp ../func.cpp ../../connectpool/connectpool.hpp ../../connectpool/connectpool.cpp ../../log/block_queue.hpp ../../log/log_sys.hpp ../../log/log_sys.cpp ../../threadpool/threadpool.hpp -o test.out -pthread -lmysqlclient -std=c++11

.PHONY:clean
clean:
	-rm test.out
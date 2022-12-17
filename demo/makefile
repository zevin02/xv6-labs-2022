#  CXX=g++
#  CXXFLAGS=-Wall -g -c -std=c++11
#  LFLAGS=-lpthread -lrt
#  TARGET=mizzo
#  OBJ=belt.o producer.o consumer.o main.o
#  IO=io.o
# # makefile也是可以使用这些变量进行替换的
# $(TARGET):$(OBJ)$(IO)	# 这些都是可以理解为一个宏--> mizzo:belt.o producer.o consumer.o main.o io.o
#   	$(CXX) -g -o $(TARGET) $(OBJ) $(IO) $(LFLAGS) # g++ -g -o mizzo belt.o .... -lpthread -lrt

# # 生成.o文件，这些文件依赖于.cpp文件
# $(OBJ):%.o: %.cpp   #%.o,%.cpp是对所有的文件而言的
# 	$(CXX) $(CXXFLAGS) $< -o $@   

# # $@表示目标可执行文件，$<表示第一个依赖文件，就是%.o, $^表示所有依赖文件
# # g++ -Wall -g -c -std=c++11 belt.o -o belt.o
# #

# $(IO):%.o:%.c
# 	$(CXX) $(CXXFLAGS) $< -O $@

# .PHONY:clean run depend	# 持久性的使用
# depend: # 依赖

# run:
# 	./$(TARGET)

# clean:
# 	rm $(TARGET) *.o

test: test.c
	gcc -o $@ $^

CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
TARGET = planificador
SRCS = tarea.cpp

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) -lpthread

clean:
	rm -f $(TARGET)

.PHONY: cleancat -A Makefile
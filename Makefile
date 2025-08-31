CXX      := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pthread
SRC      := main.cpp tcp.cpp http.cpp
OBJ      := $(SRC:.cpp=.o)
BIN      := server

# Default build is debug
all: debug

debug: CXXFLAGS += -g -O0
debug: $(BIN)

release: CXXFLAGS += -O2 -DNDEBUG
release: clean $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

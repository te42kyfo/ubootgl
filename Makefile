.PHONY: all clean

CPP_FILES := $(wildcard *.cpp) $(wildcard imgui/*.cpp)
HEADERS= $(shell find . -iname "*.hpp")
OBJ_FILES := $(addprefix obj/,$(CPP_FILES:.cpp=.o))
DEPS := $(addprefix obj/,$(CPP_FILES:.cpp=.d))

INCLUDE = -I/usr/include/SDL2 -I/usr/include/
LD_FLAGS = `sdl2-config --libs` -lSDL2_ttf -lGL -lGLEW  -fopenmp
LD_FLAGS_DEBUG = $(LD_FLAGS) -g -pg
CC_FLAGS = -std=c++14 -Wall -Wextra -fopenmp -Wno-strict-overflow -Ofast -g -march=native  -DNDEBUG
CC_FLAGS_DEBUG = -std=c++14 -Wall -Wextra -Wno-strict-overflow -g -pg -O2 -fopenmp -fno-omit-frame-pointer
NAME = ubootgl
BIN =

all: $(NAME)

run: all
	./$(NAME)

$(NAME): $(OBJ_FILES)
	g++ -o $(BIN)$@   $^ $(LD_FLAGS)

mgtest: obj/mgtest/mgtest.o obj/pressure_solver.o
	g++ -o mgtest/$@   $^ $(LD_FLAGS)

obj/%.o: %.cpp
	g++ -MMD -MP $(CC_FLAGS) $(INCLUDE) -c $< -o $@

clean:
	-rm obj/*.o $(BIN)$(NAME) obj/*.d

-include $(DEPS)


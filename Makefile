.PHONY: all clean

CPP_FILES := $(wildcard *.cpp) $(wildcard imgui/*.cpp)
HEADERS= $(shell find . -iname "*.hpp")
OBJ_FILES := $(addprefix obj/,$(CPP_FILES:.cpp=.o))
INCLUDE = -I/usr/include/SDL2
LD_FLAGS = `sdl2-config --libs` -lSDL2_ttf -lGL -lGLEW  -fopenmp
LD_FLAGS_DEBUG = $(LD_FLAGS) -g -pg
CC_FLAGS = -std=c++14 -Wall -g -Ofast -march=native -fopenmp
CC_FLAGS_DEBUG = -std=c++14 -Wall -g -pg -O2 -fopenmp -fno-omit-frame-pointer
NAME = ubootgl
BIN =

all: $(NAME)

run: all
	./$(NAME)

$(NAME): $(OBJ_FILES)
	g++ -o $(BIN)$@   $^ $(LD_FLAGS)

obj/%.o: %.cpp $(HEADERS)
	g++ $(CC_FLAGS) $(INCLUDE) -c -o $@ $<

clean:
	-rm obj/*.o $(BIN)$(NAME)

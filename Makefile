.PHONY: all clean

CPP_FILES := $(wildcard *.cpp)
HEADERS= $(shell find . -iname "*.hpp")
OBJ_FILES := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))
INCLUDE =
LD_FLAGS = `sdl2-config --libs` -lGL -lGLEW  -fopenmp
LD_FLAGS_DEBUG = `sdl2-config --libs` -lGL -lGLEW -fopenmp  -fsanitize=address -fsanitize=undefined
CC_FLAGS = -std=c++11 -Wall  $(INCLUDE) -g -Ofast -march=native -fopenmp
CC_FLAGS_DEBUG = -std=c++11 -Wall  $(INCLUDE) -g -Og -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
NAME = ubootgl
BIN =

all: $(NAME)

run: all
	./$(NAME)

$(NAME): $(OBJ_FILES)
	g++ -o $(BIN)$@   $^ $(LD_FLAGS)

obj/%.o: %.cpp $(HEADERS)
	g++ $(CC_FLAGS) -c -o $@ $<

clean:
	-rm obj/*.o $(BIN)$(NAME)

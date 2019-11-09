TARGET=mandel
OBJECTS=$(BUILD)/main.o $(BUILD)/mandelbrot.o
LIBS=-lm -lOpenCL -lSDL2 -lpthread
ARGS=-g -Wall
CLEAN=rm -f
CPPC=g++
SRC=./src
BUILD=./build

all:
	mkdir -p $(BUILD)
	make $(TARGET)

$(TARGET): $(OBJECTS)
	$(CPPC) -o $(TARGET) $(ARGS) $(OBJECTS) $(LIBS)

$(BUILD)/main.o: $(SRC)/main.cpp $(SRC)/mandelbrot.hpp
	$(CPPC) -c -o $(BUILD)/main.o $(ARGS) $(SRC)/main.cpp

$(BUILD)/mandelbrot.o: $(SRC)/mandelbrot.cpp $(SRC)/mandelbrot.hpp
	$(CPPC) -c -o $(BUILD)/mandelbrot.o $(ARGS) $(SRC)/mandelbrot.cpp

clean:
	$(CLEAN) $(OBJECTS)

cleanall:
	$(CLEAN) $(OBJECTS) $(TARGET)

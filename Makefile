CC = gcc
FLAGS = -g -O0 -Wall
TARGET = metricas_rendimiento
SOURCES = metricas_rendimiento.c
LIBS = -pthread -lrt

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(FLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)
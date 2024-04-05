SOURCES=$(shell find . -type f -iname '*.c')
TARGET=20171664.out

all: $(TARGET)

$(TARGET): $(SOURCES)
	arm-none-linux-gnueabi-gcc -static $(SOURCES) -o $(TARGET) -Wall

clean:
	rm -f $(TARGET)

push:
	adb push $(TARGET) /data/local/tmp

test:
	gcc -static $(SOURCES) -o $(TARGET) -Wall

SOURCE = app.c
TARGET = app

all: $(TARGET)

$(TARGET): $(SOURCE)
	arm-none-linux-gnueabi-gcc -static $(SOURCE) -o $(TARGET) -Wall

clean:
	rm -rf $(TARGET)

push:
	adb push $(TARGET) /data/local/tmp

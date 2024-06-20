LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:=mini-game
LOCAL_SRC_FILES:=game1/jniapi.cpp game1/renderer.cpp game1/shaders/loader.cpp
LOCAL_SRC_FILES+=game1/models/road.cpp game1/models/sword.cpp game1/models/car.cpp \
 game1/models/house.cpp game1/models/androboy.cpp
LOCAL_SRC_FILES+=game2/jniapi.cpp game2/renderer.cpp game2/shaders/loader.cpp
LOCAL_SRC_FILES+=game2/models/androman.cpp game2/models/tile.cpp
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv2

include $(BUILD_SHARED_LIBRARY)
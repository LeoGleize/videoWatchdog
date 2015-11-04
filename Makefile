
CXXFLAGS =	-O2 -g -Wall -fmessage-length=0  -std=c++11

OBJS =		IntensityAquisition.o blackmagic/cameradecklink.o blackmagic/cardSDK/DeckLinkAPIDispatch.o  \
			ServerInstance/ServerInstance.o ServerInstance/detectState.o recognition/imageRecognition.o \
			blackmagic/cardexceptions.o ServerInstance/checkImage.o ServerInstance/base64/base64.o		\
			ServerInstance/detectSound.o watchdog/hdmiWatchdog.o ServerInstance/webwatchdog.o
			

LIBS = 		`pkg-config --libs opencv` -lopencv_highgui -ldl -lpthread -fopenmp -lboost_system -lboost_thread -lcpprest

TARGET =	R7IntensityProServer

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS) $(INCDIRS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

install: 
	mv $(TARGET) /usr/bin/$(TARGET)

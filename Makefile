CXXFLAGS = -O3 -g -Wall -fmessage-length=0  -std=c++11 --fast-math

OBJS =		IntensityAquisition.o blackmagic/cameradecklink.o blackmagic/cardSDK/DeckLinkAPIDispatch.o  \
			ServerInstance/ServerInstance.o ServerInstance/detectState.o recognition/imageRecognition.o \
			blackmagic/cardexceptions.o ServerInstance/checkImage.o ServerInstance/base64/base64.o		\
			ServerInstance/detectSound.o watchdog/hdmiWatchdog.o ServerInstance/webwatchdog.o			\
			ServerInstance/tcpClient/tcpClient.o
			

LIBS = 		`pkg-config --libs opencv` -lopencv_highgui -ldl -lpthread -fopenmp -lboost_system -lboost_thread -lcpprest -lboost_random -ltesseract

TARGET =	videoWatchdog

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS) $(INCDIRS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

install: 
	mv $(TARGET) /usr/bin/$(TARGET)

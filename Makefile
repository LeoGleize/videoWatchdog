
CXXFLAGS =	-O4 -g -Wall -fmessage-length=0  -std=c++11

OBJS =		IntensityAquisition.o blackmagic/cameradecklink.o blackmagic/cardSDK/DeckLinkAPIDispatch.o \
			recognition/imSearch.o

LIBS = 		`pkg-config --libs opencv` -lopencv_highgui -ldl -lpthread -fopenmp

TARGET =	IntensityAquisition



$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS) $(INCDIRS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

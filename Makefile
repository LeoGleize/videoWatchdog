
CXXFLAGS =	-O0 -g -Wall -fmessage-length=0  

OBJS =		IntensityAquisition.o blackmagic/cameradecklink.o blackmagic/cardSDK/DeckLinkAPIDispatch.o

LIBS = 		`pkg-config --libs opencv` -lopencv_highgui -ldl -lpthread

TARGET =	IntensityAquisition



$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS) $(INCDIRS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

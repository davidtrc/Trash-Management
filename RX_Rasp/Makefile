all: Reception
	
Reception: RCSwitch.o Reception.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@ -lwiringPi
	

clean:
	$(RM) *.o Reception

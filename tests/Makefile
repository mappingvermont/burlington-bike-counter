CXX      := g++
CXXFLAGS := -std=c++17 -Wall -I../phase2

CATCH2 := $(shell brew --prefix catch2 2>/dev/null)
ifeq ($(CATCH2),)
$(error catch2 not found — run: brew install catch2)
endif
CXXFLAGS += -I$(CATCH2)/include
LDFLAGS  := -L$(CATCH2)/lib -lCatch2Main -lCatch2

.PHONY: test clean
test: test_detection
	./test_detection

test_detection: test_detection.cpp ../phase2/detection.h
	$(CXX) $(CXXFLAGS) -o $@ test_detection.cpp $(LDFLAGS)

clean:
	rm -f test_detection

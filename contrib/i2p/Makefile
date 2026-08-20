# Standalone test harness for src/i2p_sam.h
# Does NOT require building the full Dimecoin node.
REPO_ROOT ?= ../..
CXX ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -I$(REPO_ROOT)/src

TEST_BIN := test_i2p_sam
MOCK_PID :=

.PHONY: test clean

test: $(TEST_BIN)
	@python3 mock_sam.py >/tmp/mock_sam.log 2>&1 & echo $$! > /tmp/mock_sam.pid; \
	sleep 1; \
	./$(TEST_BIN); rc=$$?; \
	kill `cat /tmp/mock_sam.pid` 2>/dev/null || true; \
	exit $$rc

$(TEST_BIN): test_i2p_sam.cpp $(REPO_ROOT)/src/i2p_sam.h
	$(CXX) $(CXXFLAGS) -o $@ test_i2p_sam.cpp

clean:
	rm -f $(TEST_BIN)

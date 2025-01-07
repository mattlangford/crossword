CXX := clang++
CXXFLAGS := -std=c++20 -O3 -g

SRCS := $(wildcard *.cc)
NAMES := $(basename $(SRCS))
BUILDDIR := build

# Default: build and run everything
all: $(NAMES)

# Build and run a single file by name, e.g. `make bench`
# This target rule builds build/<name> then runs it
$(NAMES): %: $(BUILDDIR)/%
	./$<

# Compile each .cc into build/<name>
$(BUILDDIR)/%: %.cc
	mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -rf $(BUILDDIR)
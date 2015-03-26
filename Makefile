# Customizable Makefile template.
#
# This Makefile should work for projects using the Google style guide out of
# the box. Source files should be called some/dir/foo.cpp. To create a binary
# some/other/dir/mybin, simply create a source file named
# some/other/dir/mybin_main.cpp. `make' will build all such binaries by default.

# FULL_SOURCES should include all source files.
FULL_SOURCES := ${shell find . -name "*.cpp" | sed "s|^\./||"}
# FULL_HEADERS should include all header files.
FULL_HEADERS := ${shell find . -name "*.h" | sed "s|^\./||"}

# MAIN_SOURCES should be the subset of FULL_SOURCES that have main functions.
MAIN_SOURCES := ${filter %_main.cpp,${FULL_SOURCES}}

# TARGETS should include all binaries you want to generate.
TARGETS := ${patsubst %_main.cpp,%,${MAIN_SOURCES}}

# DEFAULT_BUILD_TARGETS should specify which targets to build if make is run
# with no arguments.
DEFAULT_BUILD_TARGETS := joosc test

# CXX is the C++ compiler.
CXX := /usr/local/clang-3.4/bin/clang++
# CXXFLAGS are the flags passed to the C++ compiler.
CXXFLAGS := -Wall -Wextra -std=c++11 -MMD -MP -g -pedantic -I ./ -include tools/cpp/std.h
# LDFLAGS are the flags passed to the C++ linker.
LDFLAGS := -lpthread -B third_party/bin/

# If one of the sanitizers are enabled, then append the appropriate thing to
# CXXFLAGS and LDFLAGS. Note that we do this before generating BUILD_CACHE_KEY,
# so we don't re-use values across different kinds of builds.
ifneq (${SANITIZER},)
	CXXFLAGS := ${CXXFLAGS} -fsanitize=${SANITIZER}
	LDFLAGS := ${LDFLAGS} -fsanitize=${SANITIZER}
endif

# BUILD_CACHE_KEY_FILE should contain all variables that are used when building
# your source files.  This ensures that changing a variable like CXXFLAGS will
# automatically cause a fresh compilation, rather than reusing previous
# compilation artifacts. We currently use the contents of the Makefile and the
# SANITIZER variable as the build cache key.
BUILD_CACHE_KEY_FILE := ${shell tempfile}
UNUSED := ${shell echo ${SANITIZER} >> ${BUILD_CACHE_KEY_FILE}}
UNUSED := ${shell cat Makefile >> ${BUILD_CACHE_KEY_FILE}}
BUILD_CACHE_KEY := ${shell md5sum ${BUILD_CACHE_KEY_FILE} | head -c 32}
UNUSED := ${shell rm ${BUILD_CACHE_KEY_FILE}}

# Compute some other helpful variables.
BUILD_ROOT := .build
BUILD_DIR := ${BUILD_ROOT}/${BUILD_CACHE_KEY}
TO_BUILD_DIR = ${patsubst %,${BUILD_DIR}/%,${1}}

CORE_SOURCES := ${filter-out ${MAIN_SOURCES},${FULL_SOURCES}}
FULL_OBJECTS := ${call TO_BUILD_DIR,${FULL_SOURCES:.cpp=.o}}
CORE_OBJECTS := ${call TO_BUILD_DIR,${CORE_SOURCES:.cpp=.o}}

.PHONY: default all clean format dist ${TARGETS}

default: ${DEFAULT_BUILD_TARGETS}
all: ${TARGETS}

clean:
	rm -rf ${BUILD_ROOT} ${TARGETS} submit.zip;

# Compile.
${FULL_OBJECTS}: ${BUILD_DIR}/%.o: ./%.cpp
	@mkdir -p ${dir $@};
	${CXX} ${CXXFLAGS} -c $< -o $@;

# Link.
${call TO_BUILD_DIR,${TARGETS}}:
	@mkdir -p ${dir $@};
	${CXX} ${LDFLAGS} $^ -o $@;

# Copy.
${TARGETS}: %: ${call TO_BUILD_DIR,%}
	cp $^ $@;

# Clang-format source files.
format: ${FULL_SOURCES} ${FULL_HEADERS}
	clang-format -style=Google -i ${filter-out third_party/%,$^};

# Read source-level deps.
FULL_DEPENDS := ${call TO_BUILD_DIR,${FULL_SOURCES:.cpp=.d}}
-include ${FULL_DEPENDS}

# By default, all targets depend on all possible object files.  This is a safe
# default, but will usually compile way more than you need. It would be more
# efficient to take advantage of your knowledge of your project and list deps
# more explicitly.
#
# Here's a simple example:
#   server: ${call TO_BUILD_DIR,server_main.o} ${filter-out %_test.o,${CORE_OBJECTS}}
#   test: ${call TO_BUILD_DIR,test_main.o} ${filter %_test.o,${CORE_OBJECTS}}
# 	some/dir/foobar: ${call TO_BUILD_DIR,some/dir/foobar_main.o some/dir/foo.o other/dir/bar.o}
#
# Note how we're using the TO_BUILD_DIR macro; this is important, as the deps
# need to be inside the build directory.  Don't forget to comment out the
# default behaviour if you customize this.

# Remove gtest from most targets, so we don't need to wait for it.
TEST_OBJECTS := ${filter %_test.o,${CORE_OBJECTS}}
NON_TEST_OBJECTS := ${filter-out ${TEST_OBJECTS},${CORE_OBJECTS}}

${call TO_BUILD_DIR,${TARGETS}}: %: %_main.o ${NON_TEST_OBJECTS}
${call TO_BUILD_DIR,test}: ${CORE_OBJECTS}

dist: clean
	zip -r submit.zip . -x '*.git*' -x '*.build*' -x '*third_party/cs444*'

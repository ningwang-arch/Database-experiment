# define the Cpp compiler to use
CXX = g++
# define any compile-time flags
CXXFLAGS    := -std=c++17 -Wall -Wextra -g

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS =

OUTPUT  := output

INCLUDE  := ConnectionPool/include/ SimpleHttpServer/include/  include/	/usr/include/json/ /usr/include/mysql/

SRC      := ConnectionPool/src/	SimpleHttpServer/src/	src/

LIB	 := mysqlclient	pthread	jsoncpp	boost_program_options mbedtls mbedcrypto

MAIN_SRC = main.cpp

TEST_SRC = ConnectionPool/test/main.cpp

MAIN     := main
FIXPATH = $1
RM = rm -f
MD  := mkdir -p


SOURCEDIRS  := $(shell find $(SRC) -type d)
INCLUDEDIRS := $(shell find $(INCLUDE) -type d)

LIBS		:= $(patsubst %,-l%,$(LIB:%/=%))
SOURCES     := $(wildcard $(patsubst %,%*.cpp, $(SOURCEDIRS)))
SOURCES     += $(MAIN_SRC)
INCLUDES    := $(patsubst %,-I%, $(INCLUDEDIRS:%/=%))

OBJECTS     := $(SOURCES:.cpp=.o)


OUTPUTMAIN  := $(call FIXPATH,$(OUTPUT)/$(MAIN))

all: $(OUTPUT) $(MAIN)
	@echo Executing 'all' complete!

$(OUTPUT):
	$(MD) $(OUTPUT)

$(MAIN): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(OUTPUTMAIN) $(OBJECTS) $(LFLAGS)$(LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $<  -o $@


.PHONY: clean
clean:
	$(RM) $(OUTPUTMAIN)
	$(RM) $(call FIXPATH,$(OBJECTS))
	@echo Cleanup complete!

run: all
	./$(OUTPUTMAIN)
	@echo Executing 'run: all' complete!
BUGMATIC_SOURCES = ./json11/json11.cpp \
									 ./bugmatic/bugmatic.cpp \
									 ./bugmatic/configfile.cpp \
									 ./bugmatic/fake_filesystem.cpp \
									 ./bugmatic/main.cpp \
									 ./bugmatic/md5hash.cpp \
									 ./bugmatic/url_request.cpp

BUGMATIC_INCLUDES = -I./json11 \
										-I./bugmatic

BUGMATIC_LINKER_FLAGS = -lcurl

all: dependencies clean build

dependencies:
	git submodule init
	git submodule update
	mkdir -p ./products

build:
	xcrun clang++ -x c++ -std=c++11 -arch x86_64 $(BUGMATIC_SOURCES) $(BUGMATIC_INCLUDES) $(BUGMATIC_LINKER_FLAGS) -o products/bugmatic

clean:
	-rm ./products/bugmatic

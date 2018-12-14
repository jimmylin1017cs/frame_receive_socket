CC = gcc
CXX = g++
#PYTHON_VERSION = 2.7

COMMON = 
CFLAGS = 
CXXFLAGS = 


COMMON  += -DOPENCV
CFLAGS += -DOPENCV
#CXXFLAGS  += -DOPENCV
CXXFLAGS += -std=c++11

#LDFLAGS += `pkg-config --libs opencv-3.2.0`
#COMMON  += `pkg-config --cflags opencv-3.2.0`

# link to opencv
LDFLAGS += `pkg-config --libs opencv`
COMMON  += `pkg-config --cflags opencv`

# link to python3
LDFLAGS += `pkg-config --libs python`
COMMON  += `pkg-config --cflags python`

#LDFLAGS += -lpython$(PYTHON_VERSION)
#COMMON += -I/usr/include/python$(PYTHON_VERSION)

LIBS = 
OBJDIR = ./obj/

#SRCS = 
#EXT = .cpp
#OBJ = $(SRCS:$(EXT)=.o)

OBJ = main.o mjpeg_streaming.o socket_server.o image.o
OBJS = $(addprefix $(OBJDIR), $(OBJ))
EXEC = a.out

all: mkdir_obj $(EXEC)

mkdir_obj:
	mkdir -p obj

$(EXEC): $(OBJS)
	$(CXX) $(COMMON) $(CFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJDIR)%.o: %.cpp
	$(CXX) $(COMMON) $(CFLAGS) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)%.o: %.c
	$(CC) $(COMMON) $(CFLAGS) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

cleanall:clean cleanexe

cleanobj:
	rm -rf $(OBJDIR)

cleanexe:
	rm -f $(EXEC)

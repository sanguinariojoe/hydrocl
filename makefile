# Makefile for Hydrax v0.5.3
# version of Jose Luis Cercós Pita
# Ubuntu 10.04
# GCC Compiler
# Release version
# Shared library

# ----------------------------------------
# Install prefix (default = /usr)
# ----------------------------------------
ifndef PREFIX
	PREFIX =/usr
endif

# ----------------------------------------
# Only for Debian installer purposes,
# don't use it if you don't know what are
# you doing.
# ----------------------------------------
ifndef DESTDIR
	DESTDIR =
endif

# ----------------------------------------
# OGRE Flags
# ----------------------------------------
OGRE_CFLAGS = -I$(PREFIX)/include/OGRE
OGRE_LDFLAGS = -L$(PREFIX)/lib -lOgreMain

# ----------------------------------------
# Hydrax Flags
# ----------------------------------------
HYDRAX_CFLAGS = -I$(PREFIX)/include/Hydrax
HYDRAX_LDFLAGS = -L$(PREFIX)/lib -lhydrax

# ----------------------------------------
# OpenCL Flags
# ----------------------------------------
OCL_CFLAGS = -I$(PREFIX)/include/CL -D__OpenCL__
OCL_LDFLAGS = -L$(PREFIX)/lib -lOpenCL

# ----------------------------------------
# Collect Flags
# ----------------------------------------
CFLAGS = -s -O2 -fPIC -DUNIX -c $(OGRE_CFLAGS) $(HYDRAX_CFLAGS) $(OCL_CFLAGS) -I./include/
LDFLAGS = -shared $(OGRE_LDFLAGS) $(HYDRAX_LDFLAGS) $(OCL_LDFLAGS)

# ----------------------------------------
# Compilers
# ----------------------------------------
# Detecting 64 bits version
ARCH =$(shell uname -m | grep 64)
# Verbose compiling
ifdef VERBOSE
	CC = g++
	LD = g++
	# 64 bits version
	ifneq "$(strip $(ARCH))" ""
		CC = g++ -m64
		LD = g++ -m64
	endif
	CP = cp
	RM = rm
	LN = ln
	MKDIR = mkdir
else
	CC = @g++
	LD = @g++
	# 64 bits version
	ifneq "$(strip $(ARCH))" ""
		CC = @g++ -m64
		LD = @g++ -m64
	endif
	CP = @cp
	RM = @rm
	LN = @ln
	MKDIR = @mkdir
endif

# ----------------------------------------
# Output
# ----------------------------------------
NAME=libhydrocl.so.0.5.0
OUTPUTOBJPREFIX=lib/Release/
OUTPUT = $(OUTPUTOBJPREFIX)$(NAME)
SRCOBJPREFIX = src/hydrocl/

# ----------------------------------------
# Objects
# ----------------------------------------
OBJPREFIX = obj/Release/
OBJECTS = $(OBJPREFIX)HydrOCLGrid.o $(OBJPREFIX)HydrOCLPerlin.o $(OBJPREFIX)HydrOCLUtils.o

# -------- Compiling targets -----------------------------------------------------
# all target:
# Need build all paths for objets & libraries. Then build the otuput
all: dirs $(OUTPUT)

# OUTPUT target:
# Call to compile all source files, then link it.
$(OUTPUT): $(OBJECTS)
	@echo "\033[1;1;34m Linking $(OUTPUT)... \033[0m"
	$(LD) $(LDFLAGS) $(OBJECTS) -o $(OUTPUT)
	@echo "\033[1;1;31m Built $(OUTPUT)! \033[0m"

# OBJECTS targets:
# Compile all the source files
$(OBJPREFIX)HydrOCLGrid.o:
	@echo "\033[1;1;32m Compiling $@. \033[0m"
	$(CC) $(CFLAGS) -o $@ $(SRCOBJPREFIX)HydrOCLGrid.cpp
$(OBJPREFIX)HydrOCLPerlin.o:
	@echo "\033[1;1;32m Compiling $@. \033[0m"
	$(CC) $(CFLAGS) -o $@ $(SRCOBJPREFIX)HydrOCLPerlin.cpp
$(OBJPREFIX)HydrOCLUtils.o:
	@echo "\033[1;1;32m Compiling $@. \033[0m"
	$(CC) $(CFLAGS) -o $@ $(SRCOBJPREFIX)HydrOCLUtils.cpp

# clean target:
# Remove objects/binaries
clean:
	$(RM) -rf obj lib
	@echo "\033[1;1;31m Cleaned. \033[0m"

# dirs target:
# Builds folders for the objects & binaries
dirs:
	@echo "\033[1;1;34m Creating needed paths... \033[0m"
	$(MKDIR) -p obj
	$(MKDIR) -p obj/Release
	$(MKDIR) -p lib
	$(MKDIR) -p lib/Release

# -------- Installing targets ---------------------------------------------------
# install target:
# Call to install all parts (Libraries, includes and media).
install: InstLib InstInc InstMedia
	@echo "\033[1;1;31m Installed! \033[0m"

# install library target:
# Make the folder for the library, copy the output into, 
# remove any other library with the same name, and make symbolic
# link to the library.
InstLib:
	@echo "\033[1;1;34m Installing library (into $(DESTDIR)$(PREFIX)/lib)... \033[0m"
	$(MKDIR) -p $(DESTDIR)$(PREFIX)/lib
	$(CP) $(OUTPUT) $(DESTDIR)$(PREFIX)/lib/$(NAME) 
	$(RM) -f $(DESTDIR)$(PREFIX)/lib/libhydrocl.so 
	$(LN) -s -T ./$(NAME) $(DESTDIR)$(PREFIX)/lib/libhydrocl.so

# install headers target:
# Make the destination folder, and copy all includes into.
InstInc:
	@echo "\033[1;1;34m Installing header files (into $(DESTDIR)$(PREFIX)/include/Hydrax)... \033[0m"
	$(MKDIR) -p $(DESTDIR)$(PREFIX)/include/
	$(CP) -R include/* $(DESTDIR)$(PREFIX)/include/

# install media target:
# Make the destination folder, and copy all media into.
InstMedia:
	@echo "\033[1;1;34m Installing media files (into $(DESTDIR)$(PREFIX)/share/Hydrax)... \033[0m"
	$(MKDIR) -p $(DESTDIR)$(PREFIX)/share/Hydrax/Media
	$(CP) -R Media/* $(DESTDIR)$(PREFIX)/share/Hydrax/Media

# Show a help page:
help:
	@echo "HydrOCL make file help page."
	@echo "Using:"
	@echo "\tmake [Objective] [Options]"
	@echo ""
	@echo "Valid objectives can be:"
	@echo "\thelp"
	@echo "\t\tShow this help page."
	@echo "\tclean"
	@echo "\t\tRemoves all compiled files."
	@echo "\tall"
	@echo "\t\tCompile all (Default objective)."
	@echo "\tinstall"
	@echo "\t\tInstall the libraries into $(DESTDIR)$(PREFIX)/lib, the header files into $(DESTDIR)$(PREFIX)/include, and the media files into $(DESTDIR)$(PREFIX)/share/Hydrax/Media."
	@echo "If any objective is specified, all objective will be performed."
	@echo ""
	@echo "Valid options can be:"
	@echo "\tDESTDIR=Destination directory."
	@echo "\t\tUsed to perform debian packages, can be used to install into specified directory with the same structure of root. Normal users don't want to use this variable."
	@echo "\tPREFIX=Install path. (default value = /usr)"
	@echo "\t\tPath where all data will be installed. This path must have a subfolder called lib, another called include, and another called share."
	@echo "\tVERBOSE=0/1. (default value = 0)"
	@echo "\t\tHide/Show additional info in the compile process."
	@echo ""
	@echo "Example:"
	@echo "\tmake clean"
	@echo "\tmake all"
	@echo "\tsudo make install"


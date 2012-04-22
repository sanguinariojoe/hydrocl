# makefile for Hydrax demo
# Jose Luis Cercós Pita
# Ubuntu 10.04
# GCC Compiler
# Release version

# ----------------------------------------
# Install prefix (default /usr)
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
OGRE_LDFLAGS = -L$(PREFIX)/lib -lOgreMain -lOIS

# ----------------------------------------
# Hydrax Flags
# ----------------------------------------
HYDRAX_CFLAGS = -I$(PREFIX)/include/Hydrax
HYDRAX_LDFLAGS = -L$(PREFIX)/lib -lhydrax

# ----------------------------------------
# Hydrax Flags
# ----------------------------------------
HYDROCL_CFLAGS = -I$(PREFIX)/include/hydrocl
HYDROCL_LDFLAGS = -L$(PREFIX)/lib -lhydrocl

# ----------------------------------------
# OpenCL Flags
# ----------------------------------------
OCL_CFLAGS = -I$(PREFIX)/include/CL -D__OpenCL__
OCL_LDFLAGS = -L$(PREFIX)/lib -lOpenCL

# ----------------------------------------
# Collect Flags
# ----------------------------------------
CFLAGS = -s -O2 -c $(OGRE_CFLAGS) $(HYDRAX_CFLAGS) $(HYDROCL_CFLAGS) $(OCL_CFLAGS) -I./include/
LDFLAGS = $(OGRE_LDFLAGS) $(HYDRAX_LDFLAGS) $(HYDROCL_LDFLAGS) $(OCL_LDFLAGS)

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
NAME=Demo1
OUTPUT_DIR=bin/
OUTPUT=$(OUTPUT_DIR)$(NAME)

# ----------------------------------------
# Objects
# ----------------------------------------
OBJ_DIR=obj/
OBJECTS=$(OBJ_DIR)main.o

# -------- Compiling targets -----------------------------------------------------
# all target:
# Need build all paths for objets & binaries. Then build the executable
all: dirs $(OUTPUT)

# OUTPUT target:
# Call to compile all source files, then link it.
$(OUTPUT): $(OBJECTS)
	@echo "\033[1;1;34m Linking $(OUTPUT)... \033[0m"
	$(LD) $(LDFLAGS) $(OBJECTS) -o $(OUTPUT)
	@echo "\033[1;1;31m Built $(OUTPUT)! \033[0m"

# OBJECTS targets:
# Compile all the source files
$(OBJ_DIR)main.o:
	@echo "\033[1;1;32m Compiling $@. \033[0m"
	$(CC) $(CFLAGS) src/main.cpp -o $@

# clean target:
# Remove objects/binaries
clean:
	$(RM) -rf $(OBJ_DIR)/*
	$(RM) -f $(OUTPUT_DIR)$(NAME)
	@echo "\033[1;1;31m Cleaned. \033[0m"

# dirs target:
# Builds folders for the objects & binaries
dirs:
	@echo "\033[1;1;34m Creating needed paths... \033[0m"
	$(MKDIR) -p obj
	$(MKDIR) -p bin

# -------- Installing targets ---------------------------------------------------
# install target:
# Need build all intallation paths, then copy on them.
install: installdirs
	@echo "\033[1;1;34m Installing into $(DESTDIR)$(PREFIX)/share/Hydrax/Demo1... \033[0m"
	$(CP) -rf bin $(DESTDIR)$(PREFIX)/share/Hydrax/Demo1
	$(CP) -rf include $(DESTDIR)$(PREFIX)/share/Hydrax/Demo1
	$(CP) -rf Media $(DESTDIR)$(PREFIX)/share/Hydrax/Demo1
	$(CP) -rf src $(DESTDIR)$(PREFIX)/share/Hydrax/Demo1
	$(CP) -f Demo1.cbp $(DESTDIR)$(PREFIX)/share/Hydrax/Demo1
	$(CP) -f makefile $(DESTDIR)$(PREFIX)/share/Hydrax/Demo1
	@echo "\033[1;1;31m Installed! \033[0m"

# installdirs target:
# build all intallation paths
installdirs:
	$(MKDIR) -p $(DESTDIR)$(PREFIX)/share/Hydrax/Demo1

# Show a help page:
help:
	@echo "Hydrax demostration make file help page."
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
	@echo "\t\tInstall all into $(DESTDIR)$(PREFIX)/share/Hydrax/Demo1."
	@echo "\033[1;1;31m \t\tWARNING: The executable needs permissions to write in the execution folder, so probably you want to install it into a user folder, or copy later. \033[0m"
	@echo "If any objective is specified, all objective will be performed."
	@echo ""
	@echo "Valid options can be:"
	@echo "\tDESTDIR=Destination directory."
	@echo "\t\tTo have permissions needed to execute it, probably you want to use your personal folder."
	@echo "\tPREFIX=Install path. (default value = /usr)"
	@echo "\t\tPath where all data will be installed. All data will be installed into subfolder called /share/Hydrax/Demo1."
	@echo "\tVERBOSE=0/1. (default value = 0)"
	@echo "\t\tHide/Show additional info in the compile process."
	@echo ""
	@echo "Example:"
	@echo "\tmake clean"
	@echo "\tmake all"
	@echo "\tmake install DESTDIR=~ PREFIX=/.Hydrax"


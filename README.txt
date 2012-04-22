README file for HydrOCL 0.5.0 (by Jose Luis Cercós Pita)
HydrOCL is a module for HydraX developed to run using OpenCL parallel computing library.

--- Linux users ---------------------------

You can use the makefile. Simply execute the commands

make clean
make -j4
make install

Depending your distribution, consider use PREFIX=/usr/local (default value is PREFIX=/usr)

make clean
make -j4 PREFIX=/usr/local
make install PREFIX=/usr/local

--- Linux developers ----------------------

Code::Blocks project has been provided, simply compile and use it. If you want to help the project, a git repository exist in gitorius.

http://gitorious.org/sonsilentsea/HydrOCL

Simply register on the web page, and send me a message.

--- Windows users -------------------------

* Code::Blocks & MinGW alternative.

If you use Code::Blocks & MinGW simply add MYGUI_LIB to the #define tab in the build options of Code::Blocks.


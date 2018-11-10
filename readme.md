# Port of Aedit to C

Using the source of Aedit shared on https://github.com/abiliojr/aedit.git, the code here is an attempt to port from PLM to C and to support newer operating systems.



# Current status

The port currently is aimed at the PC although some preliminary conditional compilation support for Unix/Linux has been included, but not tested.

Support for RMX and VAX has been removed on the basis that the PLM should be usable. Although some of the ISIS III and ISIS IV specific code still remains, this may be removed in future for the same reason.

The code as posted on 10th November 2018 compiles for win32 using Visual Studio 2017 community edition. Basic operations appear to work and simple small file creation / edit is possible.

Editing of large files has not been tested nor have some of the more advanced features e.g. macros.

# Known Problems

The original Aedit code does not support very long lines particularly well and although some initial changes were made to address this, they have been backed out as there are multiple cases that need to be handled including scrolling up, insertion and deletion.

There are problems with the Microsoft getch routine in some windows SDK versions including version 10.0.17134.0. Version 10.0.17763.0 works ok.

Mark Ogden

10th November 2018


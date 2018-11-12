# 
# Copyright (c) 2018 Nathan Lay (enslay@gmail.com)
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
# Nathan Lay
# Imaging Biomarkers and Computer-Aided Diagnosis Laboratory
# National Institutes of Health
# March 2017
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#######################################################################
# Introduction                                                        #
#######################################################################
StandardizeBValue is a utility that reads vendor-specific formatted
diffusion b-values and writes them to DICOM standard tag (0018,9087).
As DICOM files are ordinarily numerous, this tool supports recursively
searching for DICOM files to process.

The source code is partly based on SortDicomFiles from which this
README is based.
https://github.com/nslay/SortDicomFiles

#######################################################################
# Installing                                                          #
#######################################################################
If a precompiled version is available for your operating system, either
extract the archive where it best suits you, or copy the executable to
the desired location.

Once installed, the path to StandardizeBValue should be added to PATH.

Windows: Right-click on "Computer", select "Properties", then select
"Advanced system settings." In the "System Properties" window, look
toward the bottom and click the "Environment Variables" button. Under
the "System variables" list, look for "Path" and select "Edit." Append
the ";C:\Path\To\Folder" where "C:\Path\To\Folder\StandardizeBValue.exe"
is the path to the executable. Click "OK" and you are done.

Linux: Use a text editor to open the file ~/.profile or ~/.bashrc
Add the line export PATH="${PATH}:/path/to/folder" where
/path/to/folder/StandardizeBValue is the path to the executable. Save
the file and you are done.

StandardizeBValue can also be compiled from source. Instructions are
given in the "Building from Source" section.

#######################################################################
# Usage                                                               #
#######################################################################
Once installed, StandardizeBValue must be run from the command line. In
Windows this is accomplished using Command Prompt or PowerShell.
Unix-like environments include terminals where commands may be issued.

WINDOWS TIP: Command Prompt can be launched conveniently in a folder
holding shift and right clicking in the folder's window and selecting
"Open command window here."

StandardizeBValue accepts one or more given folders, files or DOS-wildcard
patterns to process and processes them, possibly re-writing any where the
diffusion b-value was standardized in tag (0018,9087)

StandardizeBValue .

This command processes all DICOM files in the current directory.

This works for DICOMs stored in one folder, but DICOMs are often
nested in some pre-existing file hierarchy. For example, this
ProstateX case has the following hierarchy:

Raw DICOM hierarchy from the ProstateX challenge:
ProstateX-0000/
+-- 1.3.6.1.4.1.14519.5.2.1.7311.5101.XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    +-- 1.3.6.1.4.1.14519.5.2.1.7311.5101.XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    +-- 1.3.6.1.4.1.14519.5.2.1.7311.5101.XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    +-- 1.3.6.1.4.1.14519.5.2.1.7311.5101.XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    +-- 1.3.6.1.4.1.14519.5.2.1.7311.5101.XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    +-- 1.3.6.1.4.1.14519.5.2.1.7311.5101.XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    +-- 1.3.6.1.4.1.14519.5.2.1.7311.5101.XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    +-- 1.3.6.1.4.1.14519.5.2.1.7311.5101.XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

Instead one can issue the -r flag to instruct StandardizeBValue to
search recursively. In this case, the command would be issued like

StandardizeBValue -r ProstateX-0000

You should see output that resembles the below
--- Begin snippet ---
Info: Processing 'ProstateX-0000//07-07-2011-MR prostaat kanker detectie WDSmc MCAPRODETW-05711/5-t2tsecor-03471/000014.dcm' ...
Error: Could not parse sequence name '*tse2d1_25'.
Error: Could not determine diffusion b-value (not a diffusion scan?).
Info: Processing 'ProstateX-0000//07-07-2011-MR prostaat kanker detectie WDSmc MCAPRODETW-05711/6-ep2ddifftraDYNDIST-69284/000000.dcm' ...
Info: Diffusion b-value = 800
Info: Saving standardized image to 'ProstateX-0000//07-07-2011-MR prostaat kanker detectie WDSmc MCAPRODETW-05711/6-ep2ddifftraDYNDIST-69284/000000.dcm' ...
Info: Processing 'ProstateX-0000//07-07-2011-MR prostaat kanker detectie WDSmc MCAPRODETW-05711/6-ep2ddifftraDYNDIST-69284/000001.dcm' ...
Info: Diffusion b-value = 400
Info: Saving standardized image to 'ProstateX-0000//07-07-2011-MR prostaat kanker detectie WDSmc MCAPRODETW-05711/6-ep2ddifftraDYNDIST-69284/000001.dcm' ...
Info: Processing 'ProstateX-0000//07-07-2011-MR prostaat kanker detectie WDSmc MCAPRODETW-05711/6-ep2ddifftraDYNDIST-69284/000002.dcm' ...
Info: Diffusion b-value = 50
Info: Saving standardized image to 'ProstateX-0000//07-07-2011-MR prostaat kanker detectie WDSmc MCAPRODETW-05711/6-ep2ddifftraDYNDIST-69284/000002.dcm' ...
--- End snippet ---

Lastly, StandardizeBValue provides the below usage message when
provided with the -h flag or no arguments. It's useful if you
forget.

Usage: ./StandardizeBValue [-hr] path|filePattern [path2|filePattern2 ...]

Options:
-h -- This help message.
-r -- Recursively search folders.

#######################################################################
# Building from Source                                                #
#######################################################################
To build StandardizeBValue from source, you will need a recent version of
CMake, a C++11 compiler, and InsightToolkit version 4 or later.

First extract the source code somewhere. Next create a separate
directory elsewhere. This will serve as the build directory. Run CMake
and configure the source and build directories as chosen. More
specifically

On Windows:
- Run cmake-gui (Look in Start Menu) and proceed from there.

On Unix-like systems:
- From a terminal, change directory to the build directory and then
run:

ccmake /path/to/source/directory

In both cases, "Configure." If you encounter an error, set ITK_DIR
and then run "Configure" again. Then select "Generate." On Unix-like
systems, you may additionally want to set CMAKE_BUILD_TYPE to "Release"

NOTE: ITK_DIR should be set to the cmake folder in the ITK lib
folder. For example: /path/to/ITK/lib/cmake/ITK-4.13/

Visual Studio:
- Open the solution in the build directory and build StandardizeBValue.
Make sure you select "Release" mode.

Unix-like systems:
- Run the "make" command.

StandardizeBValue has been successfully built and tested with:
Microsoft Visual Studio 2017 on Windows 10 Professional
Clang 6.0.1 on FreeBSD 11.2-STABLE

using ITK versions:
ITK 4.9
ITK 4.13

#######################################################################
# Caveats                                                             #
#######################################################################
DOS-wildcard patterns may not work properly when matching subfolders.
For example: /path/to/*/folder

This mostly affects Windows users since Unix shells already expand 
these.

Using absolute paths to Windows shares (i.e. \\name\folder) could cause
problems since BaseName() and DirName() have not yet implemented
parsing these kinds of paths.


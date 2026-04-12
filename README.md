# cad-viewer

A few useful notes...

### Tidy-up...

The "third_party" content has been removed from the repo as this should not really be included (and adds a significant bloat to the repo). This should instead be downloaded separately and then your project setup to reference it (more details below).

Unnecessary intermediary build files have also been removed as these should not be part of the repo (e.g. obj files and other temporary files that are generated during compilation/build).

### What you need...

> [!NOTE]
> This uses Visual Studio 2026 (works with the free Community Edition).

Download the following file from OpenCascade:

https://github.com/Open-Cascade-SAS/OCCT/releases/download/V7_9_3/opencascade-7.9.3-vc14-64-with-debug-combined.zip

The zip file contains all the OpenCascade dependencies you need as well as third-party libs. These should not exist in this repo (only your source code!)

Extract the two folders in the zip file to your project folder so that they sit alongside the existing project folders (e.g.):

- C:\cad-viewer\
  - opencascade-7.9.3-vc14-64\
  - 3rdparty-vc14-64\
  - (existing project folders...)

Your complete project folder should look something like this:

- C:\cad-viewer\
  - opencascade-7.9.3-vc14-64\
  - 3rdparty-vc14-64\
  - 3rdparty-other\
  - assets\
  - src\
  - .gitignore
  - basic_occt_gl_viewer.sln
  - etc...

> [!TIP]
> The "third-party" folder is slightly different as it contains pure source dependencies that _need_ to be included in your repo. They are small and likely won't need touching again for a while (if at all). You can ignore them for now!

### Building the app

If I have set it up right, you should be able to select x64 and Debug/Release and build the solution without error (famous last words).

On a successful build, there is a post-build step that copies required DLL files to the same location as the built executable - so that you can run it from Windows Explorer or directly from Visual Studio by hitting F5 (or whatever you have your hotkey set as for build/run).

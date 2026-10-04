# PMX source access

PMX is licensed under GNU AGPL version 3. You may use, study, modify and redistribute it under that licence. There is no warranty.

Every downloadable binary must be accompanied by its matching PMX-source.zip and BUILD-INFO.txt. The source archive contains the application source, build and installer scripts, exact pinned JUCE and NAM dependency source, NAM subdependencies, and licence notices. BUILD-INFO.txt identifies the source commit. Do not distribute a binary without providing equivalent access to its corresponding source.

Repository: https://github.com/KaranBarua01/PMX

Build instructions are in README.md. For an offline build from the source bundle, use the included dependencies:

~~~powershell
cmake -S . -B build -A x64 -DPMX_BUILD_GUI=ON -DPMX_BUILD_TESTS=ON -DFETCHCONTENT_SOURCE_DIR_JUCE=deps/JUCE -DFETCHCONTENT_SOURCE_DIR_NAMCORE=deps/NAMCore
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
~~~

The source archive excludes Git metadata and build products. Upstream code is unmodified; PMX's dependency build integration is in cmake/Dependencies.cmake.


proj = hf2 # Project name
srcs = src/main.cpp # Source files to track changes
basedir = target

nprocs = $(shell expr $(shell nproc) - 1) # Parallel compiling
ifeq ($(strip ${nprocs}), 0)
	nprocs = 1 # `-j` does not take 0 as a valid jobs count
endif
makeflags = -j${nprocs}

cmakeflags = -DCMAKE_EXPORT_COMPILE_COMMANDS=1

# Use release-with-debug-info build by default
default: relwithdebinfo

relwithdebinfo: ${srcs} CMakeLists.txt
	cmake -S . -B ${basedir}/$@ ${cmakeflags} -DCMAKE_BUILD_TYPE="RelWithDebInfo"
	@ln -sf $(shell pwd)/${basedir}/$@/compile_commands.json ${basedir}
	@make -C ${basedir}/$@ ${makeflags}
minsizerel: ${srcs} CMakeLists.txt
	cmake -S . -B ${basedir}/$@ ${cmakeflags} -DCMAKE_BUILD_TYPE="MinSizeRel"
	@ln -sf $(shell pwd)/${basedir}/$@/compile_commands.json ${basedir}
	@make -C ${basedir}/$@ ${makeflags}
release: ${srcs} CMakeLists.txt
	cmake -S . -B ${basedir}/$@ ${cmakeflags} -DCMAKE_BUILD_TYPE="Release"
	@ln -sf $(shell pwd)/${basedir}/$@/compile_commands.json ${basedir}
	@make -C ${basedir}/$@ ${makeflags}
debug: ${srcs} CMakeLists.txt
	cmake -S . -B ${basedir}/$@ ${cmakeflags} -DCMAKE_BUILD_TYPE="Debug"
	@ln -sf $(shell pwd)/${basedir}/$@/compile_commands.json ${basedir}
	@make -C ${basedir}/$@ ${makeflags}
clean:
	-@make -C ${basedir}/relwithdebinfo clean
	-@make -C ${basedir}/minsizerel clean
	-@make -C ${basedir}/release clean
	-@make -C ${basedir}/debug clean
purge:
	-rm -rf ${basedir}

.PHONY: clean debug default minsizerel purge release relwithdebinfo

# vim: set ft=make:

# Author: Blurgy <gy@blurgy.xyz>
# Date:   May 21 2021, 10:49 [CST]

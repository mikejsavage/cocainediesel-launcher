all: debug
.PHONY: debug asan release clean

EXE = linux

WSLENV ?= notwsl
ifndef WSLENV
	EXE = exe
else
	ifeq ($(shell uname -s),Darwin)
		EXE = macos
	endif
endif

debug:
	@scripts/lua.$(EXE) make.lua > build.ninja
	@scripts/ninja.$(EXE) -k 0

asan:
	@scripts/lua.$(EXE) make.lua asan > build.ninja
	@scripts/ninja.$(EXE) -k 0

release:
	@scripts/lua.$(EXE) make.lua release > build.ninja
	@scripts/ninja.$(EXE) -k 0

clean:
	@scripts/lua.$(EXE) make.lua debug > build.ninja
	@scripts/ninja.$(EXE) -t clean || true
	@scripts/lua.$(EXE) make.lua asan > build.ninja || true
	@scripts/ninja.$(EXE) -t clean || true
	@rm -rf build release
	@rm -f -- *.exp *.ilk *.ilp *.pdb game/*.exp game/*.ilk game/*.ilp game/*.pdb
	@rm -f build.ninja

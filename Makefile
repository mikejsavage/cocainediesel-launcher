all: debug
.PHONY: debug asan release clean

LUA = scripts/lua.linux
NINJA = scripts/ninja.linux

WSLENV ?= notwsl
ifndef WSLENV
	LUA = scripts/lua.exe
	NINJA = scripts/ninja.exe
endif

debug:
	@$(LUA) make.lua > build.ninja
	@$(NINJA)

asan:
	@$(LUA) make.lua asan > build.ninja
	@$(NINJA)

release:
	@$(LUA) make.lua release > build.ninja
	@$(NINJA)

clean:
	@$(LUA) make.lua debug > build.ninja
	@$(NINJA) -t clean || true
	@$(LUA) make.lua asan > build.ninja || true
	@$(NINJA) -t clean || true
	@rm -rf build release
	@rm -f *.exp *.ilk *.ilp *.pdb game/*.exp game/*.ilk game/*.ilp game/*.pdb
	@rm -f build.ninja

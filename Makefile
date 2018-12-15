all: debug
.PHONY: debug asan release clean

LUA = scripts/lua.linux
NINJA = scripts/ninja.linux
ifeq ($(OS),Windows_NT)
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
	@$(LUA) make.lua asan > build.ninja
	@$(NINJA) -t clean || true
	@rm -rf build release
	@rm -f build.ninja

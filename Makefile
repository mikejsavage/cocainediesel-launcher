all: debug
.PHONY: debug asan release clean

debug:
	@scripts/lua.linux make.lua > build.ninja
	@scripts/ninja.linux

asan:
	@scripts/lua.linux make.lua asan > build.ninja
	@scripts/ninja.linux

release:
	@scripts/lua.linux make.lua release > build.ninja
	@scripts/ninja.linux

clean:
	@scripts/lua.linux make.lua debug > build.ninja
	@scripts/ninja.linux -t clean || true
	@scripts/lua.linux make.lua asan > build.ninja
	@scripts/ninja.linux -t clean || true
	@rm -rf build release
	@rm -f build.ninja

CC = clang++
CFLAGS = -std=c++20\
		 -Werror -Wall -pedantic\
		 -DENABLE_LOGS

RELEASE_FLAGS = -O3 -DNDEBUG
DEBUG_FLAGS   = -MJ tmp.json\
				-fsanitize=undefined,address\
				-O0 -g


ddl: dedalo.cpp
	@echo "Compiling..."
	@time $(CC) $(CFLAGS) $(DEBUG_FLAGS) dedalo.cpp -o ddl
	@echo '[' > compile_commands.json
	@cat tmp.json >> compile_commands.json
	@echo ']' >> compile_commands.json
	@rm tmp.json

run: dedalo
	./ddl run

rebuild:
	$(MAKE) clean
	$(MAKE) ddl

release:
	$(MAKE) clean
	@echo "Compiling for release..."
	@time $(CC) $(CFLAGS) $(RELEASE_FLAGS) dedalo.cpp -o ddl

debug:
	$(MAKE) rebuild

clean:
	rm -rf ddl compile_commands.json *.dSYM

reset:
	@echo "Cleaning Dedalo itself..."
	rm -rf ddl compile_commands.json *.dSYM
	@echo "Cleaning Dedalo's output..."
	rm -rf dependencies tests src build build.cpp

install:
	$(MAKE) release
	cp ddl /usr/local/bin
	cp dedalo.cpp /usr/local/include

uninstall:
	rm /usr/local/bin/ddl
	rm /usr/local/include/dedalo.cpp

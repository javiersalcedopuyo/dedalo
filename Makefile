CC = clang++
CFLAGS = -std=c++20\
		 -MJ tmp.json\
		 -Werror -Wall -pedantic\
		 -fsanitize=undefined,address\
		 -O0 -g

ddl: dedalo.cpp
	@echo "Compiling..."
	@time $(CC) $(CFLAGS) dedalo.cpp -o ddl
	@echo '[' > compile_commands.json
	@cat tmp.json >> compile_commands.json
	@echo ']' >> compile_commands.json
	@rm tmp.json

run: dedalo
	./ddl run

rebuild:
	$(MAKE) clean
	$(MAKE) ddl

clean:
	rm -rf ddl compile_commands.json *.dSYM

reset:
	@echo "Cleaning Dedalo itself..."
	rm -rf ddl compile_commands.json *.dSYM
	@echo "Cleaning Dedalo's output..."
	rm -rf dependencies tests src build build.cpp

install:
	$(MAKE) rebuild
	cp ddl /usr/local/bin
	cp dedalo.cpp /usr/local/include

uninstall:
	rm /usr/local/bin/ddl
	rm /usr/local/include/dedalo.cpp

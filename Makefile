CC = clang++
CFLAGS = -std=c++20\
		 -MJ tmp.json\
		 -Werror -Wall -pedantic\
		 -fsanitize=undefined,address\
		 -O0 -g

dedalo: dedalo.cpp
	@echo "Compiling..."
	@time $(CC) $(CFLAGS) dedalo.cpp -o dedalo
	@echo '[' > compile_commands.json
	@cat tmp.json >> compile_commands.json
	@echo ']' >> compile_commands.json
	@rm tmp.json

run: dedalo
	./dedalo

clean:
	rm -rf dedalo compile_commands.json *.dSYM

reset:
	@echo "Cleaning Dedalo itself..."
	rm -rf dedalo compile_commands.json *.dSYM
	@echo "Cleaning Dedalo's output..."
	rm -rf dependencies tests src build build.cpp

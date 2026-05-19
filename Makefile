default:
	clang++ main.cpp -o build/main
run:
	./build/main
make:
	clang++ main.cpp -o build/main
	./build/main

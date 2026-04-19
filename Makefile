BUILD_DIR := build
CMAKE := cmake

.PHONY: all configure build test run-server run-client server client clean

all: build

configure:
	$(CMAKE) -S . -B $(BUILD_DIR)

build: configure
	$(CMAKE) --build $(BUILD_DIR)

test: build
	cd $(BUILD_DIR) && ctest --output-on-failure

run-server: build
	mkdir -p data logs
	./$(BUILD_DIR)/netcourse_server 9090 data/users.db logs/server.log

run-client: build
	./$(BUILD_DIR)/netcourse_client 127.0.0.1 9090

server: run-server

client: run-client

clean:
	rm -rf $(BUILD_DIR)

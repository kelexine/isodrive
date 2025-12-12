BUILD_DIR = build

all: $(BUILD_DIR)
	@cmake --build $(BUILD_DIR)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@cmake -S . -B $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)

install: all
	@cmake --install $(BUILD_DIR)

test: all
	@cd $(BUILD_DIR) && ctest --output-on-failure

magisk: all
	@cmake --build $(BUILD_DIR) --target magisk

.PHONY: all clean install test magisk

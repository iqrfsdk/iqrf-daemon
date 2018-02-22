.PHONY: build build-debug build-daemon build-daemon-debug install

build:
	@echo "Downloading libs ..."
	@python3 download-libs.py -l ../../libs
	@echo "Building libs ..."
	@python3 build-libs.py -g make -l ../../libs
	@echo "Building daemon ..."
	@python3 build-daemon.py -g make -l ../../libs

build-debug:
	@echo "Downloading libs ..."
	@python3 download-libs.py -l ../../libs
	@echo "Building libs for debug ..."
	@python3 build-libs.py -g make -l ../../libs -d yes
	@echo "Building daemon for debug ..."
	@python3 build-daemon.py -g make -l ../../libs -d yes

build-daemon:
	@echo "Building daemon ..."
	@python3 build-daemon.py -g make -l ../../libs

build-daemon-debug:
	@echo "Building daemon for debug ..."
	@python3 build-daemon.py -g make -l ../../libs -d yes

install:
	@echo "Installing daemon ..."
	@python3 install-daemon.py

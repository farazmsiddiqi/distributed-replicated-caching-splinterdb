DOCKER_REPO = neilk3
IMAGE_BUILD_ENV_BASE = $(DOCKER_REPO)/splinterdb-build-env
IMAGE_RUN_ENV_BASE   = $(DOCKER_REPO)/splinterdb-run-env
IMAGE_BUILD_ENV      = $(DOCKER_REPO)/replicated-splinterdb-dev
IMAGE_RUN_ENV        = $(DOCKER_REPO)/replicated-splinterdb

SPLINTERDB_ROOT = third-party/splinterdb

SRC_DIR 	= src
APPS_DIR 	= apps
INCLUDE_DIR = include

dev: $(IMAGE_BUILD_ENV)
	docker run -it --rm \
		-v `pwd`/include:/work/include \
		-v `pwd`/apps:/work/apps \
		-v `pwd`/src:/work/src \
		-v `pwd`/third-party/splinterdb:/work/splinterdb \
		-v `pwd`/docker/CMakeLists.txt:/work/CMakeLists.txt \
		-v `pwd`/docker/build-dev:/work/build/build \
		-v `pwd`/.cache:/cachepages \
		$(IMAGE_BUILD_ENV)

run: $(IMAGE_RUN_ENV)
	docker run -it --network="host" --rm $(IMAGE_RUN_ENV)

$(IMAGE_BUILD_ENV): $(IMAGE_BUILD_ENV_BASE)
	docker build -t $@ -f Dockerfile.dev .

$(IMAGE_RUN_ENV): $(IMAGE_BUILD_ENV_BASE) $(IMAGE_RUN_ENV_BASE)
	docker build -t $@ -f Dockerfile.run .

$(IMAGE_BUILD_ENV_BASE):
	docker build -t $@ -f $(SPLINTERDB_ROOT)/Dockerfile.build-env $(SPLINTERDB_ROOT)

$(IMAGE_RUN_ENV_BASE):
	docker build -t $@ -f $(SPLINTERDB_ROOT)/Dockerfile.run-env $(SPLINTERDB_ROOT)

format:
	find $(APPS_DIR) $(INCLUDE_DIR) $(SRC_DIR) \
		-type f \( -iname \*.cpp -o -iname \*.hpp -o -iname \*.h \) | \
		xargs clang-format -i

submodules:
	git submodule update --init --recursive
	chmod +x docker/build*
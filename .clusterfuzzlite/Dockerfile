FROM ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder-lite:latest AS LITE_BUILDER
FROM gcr.io/oss-fuzz-base/base-builder:v1
RUN apt-get update && apt-get install -y libbsd-dev
COPY . $SRC/app-stellar
WORKDIR $SRC/app-stellar
COPY .clusterfuzzlite/build.sh $SRC/

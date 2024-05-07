FROM ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder-lite:latest AS LITE_BUILDER

# Base image with clang toolchain
FROM gcr.io/oss-fuzz-base/base-builder:v1

RUN apt-get update && apt-get install -y libbsd-dev

# Add libbsd to ld path
ENV LD_LIBRARY_PATH=/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
RUN ldconfig

# Copy the project's source code.
COPY . $SRC/app-stellar
COPY --from=LITE_BUILDER /opt/ledger-secure-sdk $SRC/app-stellar/BOLOS_SDK

# Working directory for build.sh
WORKDIR $SRC/app-stellar

# Copy build.sh into $SRC dir.
COPY .clusterfuzzlite/build.sh $SRC/

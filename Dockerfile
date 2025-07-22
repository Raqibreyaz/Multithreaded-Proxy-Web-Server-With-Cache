FROM gcc:13-bookworm AS builder

# this will be working dir
WORKDIR /app

# Copy entire project including code + data files
COPY . .

# install libssl-dev for -lssl -lcrypto
# rm -rf for removing  package metadata as it is no longer required
RUN apt-get update && \
    apt-get install -y libssl-dev && \
    rm -rf /var/lib/lists/*

# Build the project
RUN make all


# using a lightweight os for running executable
FROM debian:bookworm-slim

WORKDIR /app

# installing runtime ssl-libs only (not-dev versions)
RUN apt-get update && \
    apt-get install -y libssl3 && \
    rm -rf /var/lib/apt/lists/*

# copying the executable to our new environment
COPY --from=builder /app/server .

# copying blocked-sites file
COPY --from=builder /app/blocked-sites.json .

# copying static files dir like home page, favicon.ico
COPY --from=builder /app/static ./static

# Expose the port
EXPOSE 4040

# Run the compiled binary
CMD ["./server"]

# Step 1: GCC compiler ke saath base image
FROM gcc:13 AS builder

WORKDIR /app

# Poora project copy karo
COPY . .

# Makefile se build karo
RUN make

# Step 2: Minimal image
FROM debian:bookworm-slim

# OpenSSL library install karo (runtime ke liye)
RUN apt-get update && apt-get install -y libssl-dev && rm -rf /var/lib/apt/lists/*

WORKDIR /server

# Final binary copy karo
COPY --from=builder /app/server .

# Port expose karo (agar server 8080 pe chal raha hai)
EXPOSE 8080

# Binary run karo
CMD ["./server"]

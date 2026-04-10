FROM debian:bookworm-slim AS builder
RUN apt-get update && apt-get install -y build-essential
WORKDIR /app
COPY . .
RUN make

FROM debian:bookworm-slim
WORKDIR /app
COPY --from=builder /app/build/server .
COPY --from=builder /app/www ./www/
EXPOSE 8899
CMD ["./server"]
FROM alpine:latest AS builder
RUN apk add --no-cache build-base
WORKDIR /app
COPY . .
RUN make

FROM alpine:latest
WORKDIR /app
COPY --from=builder /app/server .
EXPOSE 8080
CMD ["./server"]
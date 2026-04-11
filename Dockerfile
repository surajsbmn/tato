FROM alpine:3.23.3 AS builder
RUN apk add --no-cache build-base
WORKDIR /app
COPY . .
RUN make

FROM alpine:3.23.3
WORKDIR /app
COPY --from=builder /app/build/server .
COPY --from=builder /app/www ./www/
EXPOSE 8899
CMD ["./server"]
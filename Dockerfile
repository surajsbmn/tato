FROM alpine:latest AS builder
RUN apk add --no-cache build-base
WORKDIR /app
COPY . .
RUN make

FROM alpine:latest
RUN apk add --no-cache libgcc
WORKDIR /app
COPY --from=builder /app/server .
COPY --from=builder /app/www ./www/
EXPOSE 8899
CMD ["./server"]
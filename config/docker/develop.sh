cd develop
docker buildx build --push --platform=linux/amd64,linux/arm64 -t ghcr.io/matthew-mcraven/pepp/dev:$VERSION .
docker buildx build --target output-wasm --push --platform=linux/amd64,linux/arm64 -t ghcr.io/matthew-mcraven/pepp/dev:$VERSION-wasm .

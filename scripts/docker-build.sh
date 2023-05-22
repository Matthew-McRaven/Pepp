# Source this from the root
cd scripts
docker buildx build --platform linux/amd64,linux/arm64 -t registry.gitlab.com/pep10/pepsuite/develop:v0.4.3 --push .


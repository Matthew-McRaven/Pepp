
group "default" {
  targets = ["gcc", "dev-gcc", "wasm", "wasm-dbg", "dev-wasm-dbg", "clang"]
}

variable "VERSION" {
  default = "v0.15.0"
}

target "gcc" {
  context = "./develop"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/gcc:${VERSION}"]
  platforms = ["linux/amd64", "linux/arm64"]
  target = "output-gcc"
}

target "dev-gcc" {
  context = "./develop"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/dev-gcc:${VERSION}"]
  platforms = ["linux/amd64", "linux/arm64"]
  target = "output-gcc"
  args {
    BASE_IMAGE="mcr.microsoft.com/devcontainers/base:ubuntu"
  }
}

target "wasm" {
  context = "./develop"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/wasm:${VERSION}"]
  platforms = ["linux/amd64"]
  target = "output-wasm"
}

target "wasm-dbg" {
  context = "./develop"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/wasm-dbg:${VERSION}"]
  platforms = ["linux/amd64"]
  target = "output-wasm"
  args {
    QT_WASM_XARGS = "-sanitize address  -sanitize undefined -device-option QT_WASM_SOURCE_MAP=1"
  }
}

target "dev-wasm-dbg" {
  context = "./develop"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/dev-wasm-dbg:${VERSION}"]
  platforms = ["linux/amd64"]
  target = "output-gcc"
  args {
    BASE_IMAGE = "mcr.microsoft.com/devcontainers/base:ubuntu"
    QT_WASM_XARGS = "-sanitize address  -sanitize undefined -device-option QT_WASM_SOURCE_MAP=1"
  }
}

target "clang" {
  context = "./develop"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/clang:${VERSION}"]
  platforms = ["linux/amd64", "linux/arm64"]
  target = "output-clang"
}

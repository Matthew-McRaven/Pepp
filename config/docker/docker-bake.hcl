
group "default" {
  targets = ["gcc", "dev-gcc", "wasm", "wasm-dbg", "dev-wasm-dbg", "clang", "image_utils", "gcc-riscv"]
}
group "cpp" {
  targets = ["gcc", "dev-gcc", "wasm", "wasm-dbg", "dev-wasm-dbg", "clang", "gcc-riscv" ]
}
group "cross" {
  targets = ["gcc-riscv" ]
}
group "img" {
  targets = ["image_utils" ]
}


variable "VERSION" {
  default = "v0.17.0"
}

target "gcc-riscv" {
  context = "./cpp_build"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/gcc-riscv:${VERSION}"]
  platforms = ["linux/amd64", "linux/arm64"]
  target = "output-gcc-riscv"
}

target "gcc" {
  context = "./cpp_build"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/gcc:${VERSION}"]
  platforms = ["linux/amd64", "linux/arm64"]
  target = "output-gcc"
}

target "dev-gcc" {
  context = "./cpp_build"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/dev-gcc:${VERSION}"]
  platforms = ["linux/amd64", "linux/arm64"]
  target = "output-gcc"
  args {
    BASE_IMAGE="mcr.microsoft.com/devcontainers/base:ubuntu"
  }
}

target "wasm" {
  context = "./cpp_build"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/wasm:${VERSION}"]
  platforms = ["linux/amd64"]
  target = "output-wasm"
}

target "wasm-dbg" {
  context = "./cpp_build"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/wasm-dbg:${VERSION}"]
  platforms = ["linux/amd64"]
  target = "output-wasm"
  args {
    QT_WASM_XARGS = "-sanitize address  -sanitize undefined -device-option QT_WASM_SOURCE_MAP=1"
  }
}

target "dev-wasm-dbg" {
  context = "./cpp_build"
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
  context = "./cpp_build"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/clang:${VERSION}"]
  platforms = ["linux/amd64", "linux/arm64"]
  target = "output-clang"
}

target "image_utils" {
  context = "./image_utils"
  dockerfile = "Dockerfile"
  tags = ["ghcr.io/matthew-mcraven/pepp/image-utils:${VERSION}"]
  platforms = ["linux/amd64"]
  target = "output"
}

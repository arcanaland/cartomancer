build:
  rm -rf build
  mkdir build
  go build -o build/cartomancer ./...

release version="0.2.0":
  git tag v{{version}}
  git push origin v{{version}}

release-local version="0.2.0":
  #!/bin/bash
  . .env
  git tag v{{version}}
  goreleaser release


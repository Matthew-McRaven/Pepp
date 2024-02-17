#
# Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

docker buildx build --target output-gcc --push --platform=linux/amd64,linux/arm64 -t ghcr.io/matthew-mcraven/pepp/dev-gcc:v0.10.0 develop
docker buildx build --target output-wasm --push --platform=linux/amd64 -t ghcr.io/matthew-mcraven/pepp/dev-wasm:v0.10.0 develop
docker buildx build --target output-clang --push --platform=linux/amd64,linux/arm64 -t ghcr.io/matthew-mcraven/pepp/dev-clang:v0.10.0 develop
docker buildx build --push --platform=linux/amd64 -t ghcr.io/matthew-mcraven/pepp/docs:v0.10.0 docs

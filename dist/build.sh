#!/bin/bash

if [ $(uname) == "Darwin" ]; then
  platform=osx
  platform_extras=""
else
  platform=linux
  platform_extras="stun.service"
fi

pushd "$(dirname "$0")"
cd ..
buck build @.buckconfig.release :main
cp buck-out/gen/main dist/stun
cd dist
zip stun-$platform.zip stun $platform_extras
popd

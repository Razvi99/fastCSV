./bin/gn gen build

cd build
../bin/ninja

cp obj/libzlib_chromium.a ../

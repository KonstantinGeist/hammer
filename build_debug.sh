mkdir tmp_build
cd tmp_build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j4
cp hammer_tests ../bin


#cp user-config.jam boost/
echo "using python : 3.8 : ../../build/python3/usr/local/bin/python3 : ../../build/python3/usr/local/include/python3.8 : ../../build/python3/usr/local/lib/python3.8 : ;" > boost/user-config.jam
cd boost && ./bootstrap.sh --prefix=../../build/boost --with-python-version=3.9 && \
./b2 cxxflags="-fPIC -std=c++17" --prefix=../../build/boost --user-config=./user-config.jam address-model=64 link=static runtime-link=static -j 20 threading=multi install

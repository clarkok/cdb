A simple but fast SQL database. In development.

## Building

```
git clone https://github.com/clarkok/cdb.git
cd cdb
mkdir build
cd build
cmake ../
make
```

## Testing

Dependence: `gtest-1.7.0` `google-proftools`

```
cmake -DTest=ON
make test

# if you want to see details
./test/test-main
```

## Counting test Coverage

Dependence: `gtest-1.7.0` `google-proftools` `gcov`, `lcov`

```
cmake -DTest=ON -DCoverage
make coverage
open coverage/index.html
```

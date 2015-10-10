A simple but fast SQL database. In development.

## Building

Dependence: `gtest-1.7.0`, `gcov`, `lcov`

```
git clone https://github.com/clarkok/cdb.git
cd cdb
mkdir build
cd build
cmake ../
make
```

## Testing

```
# under cdb/build or somewhere else
make test

# if you want to see details, run under cdb/build
./test/test-main
```

## Counting test Coverage

```
# under cdb/build
make coverage
open coverage/index.html
```

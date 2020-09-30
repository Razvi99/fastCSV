# fastCSV
```C++
template<int max_columns, class ReadBuffer = RawReadBuffer>
class FastCSV;
```
This means that the FastCsv class takes 1 required template argument and one optional template argument.

The first argument, `max_columns`, needs to be >= than number of columns of the CSV file. Ideally, it should be equal to the number of columns, but only a little bit of space is lost if it is greater.

The second argument defaults to `RawReadBuffer`, which is the raw file reader. Think of this argument as a replaceable part of code that deals with reading from files.

The second argument can also be `GzipReadBuffer`, which would be used with <i>.gz</i> files.

## general tips
* not completely csv-standard compliant, it <b>does NOT deal with quoted columns</b>.
* uses <b>SIMD</b> (AVX2) instructions if available, to process 64 characters at once.
* uses Cloudflare's zlib implementation for best inflate performance
* a FastCSV object should be heap-allocated with new(), as it uses more than 1MB of memory - a bit to much for the stack.
* iterating a csv object row by row should be done with range-based for loops, for easier syntax and equal efficiency.
* indexing a row is as easy as `row[COLUMN_INDEX]`. This returns a `std::string_view` (>C++17) object, which contains a pointer to the beginning of the original data and a size variable.
* row parsing into columns is done when the iterator is incremented. Column accesses are O(1).
* works with negative column indexes: `row[-2]` returns the second to last column.
* using the `sv` operator to construct a `std::string_view` from a string literal avoids a call to strlen() in debug (not optimised) builds. In reality, any build with at least `-O1` optimisation produces exactly the same binary, whether or not `sv` is used.

## header parsing
The constructor of a FastCSV object can optionally receive an initializer list of `string_view` and `int&` pairs.
It will try to find the column name in the first row of the csv, and set the given variable to the index of that column.
If the column with the requested name does not exist, the variable is set to `-1`.
```C++
int some_variable_name, badly_named_var, crazy_cool_column;

auto csv = new FastCSV<420, GzipReadBuffer>("/path/to/data.csv.gz", {
        {"some_column_name",  some_variable_name},
        {"other_column_name", badly_named_var},
        {"crazy_cool_column", crazy_cool_column},
});

// then the he following is guaranteed for all column name & variable pairs
if (some_variable_name != -1)
    assert(csv->getRow()[some_variable_name] == "some_column_name");
```

## raw file example
```C++
auto csv = new FastCSV<500, RawReadBuffer>("/path/to/data.csv");

for (const auto &row : *csv) {
    if (row[0] == "-1")
        // code
    else if(row[-2].empty())
        // code
    else
        // code
}

delete csv;
```

## gzip file example
```C++
// the ONLY change is on this next line:
auto csv = new FastCSV<500, GzipReadBuffer>("/path/to/data.csv.gz");

for (const auto &row : *csv) {
    if (row[0] == "-1")
        // code
    else if(row[-2].empty())
        // code
    else
        // code
}

delete csv;
```

### usage
For gzip, Cloudflare's implementation of zlib is included in `lib/zlib`. To build it, run `lib/zlib/build.sh`.

Simply include the 3 header files and link the zlib library to use FastCSV.

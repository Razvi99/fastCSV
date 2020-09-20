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
* a FastCSV object should be heap-allocated with new(), as it uses more than 1MB of memory - a bit to much for the stack.
* iterating a csv object row by row should be done with range-based for loops, for easier syntax and equal efficiency.
* indexing a row is as easy as `row[COLUMN_INDEX]`. This returns a `std::string_view` (>C++17) object, which contains a pointer to the beginning of the original data and a size variable.
* row parsing into columns is done when the iterator is incremented. Column accesses are O(1).

## raw file example
```C++
auto csv = new FastCSV<500, RawReadBuffer>("/path/to/data.csv");

for (auto row : *csv) {
    if (row[0] == "-1")
        // code
    else if(row[0] == "0")
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

for (auto row : *csv) {
    if (row[0] == "-1")
        // code
    else if(row[0] == "0")
        // code
    else
        // code
}

delete csv;
```

### usage in project
All files in the `lib/` folder need to be included for raw + gzip functionality.

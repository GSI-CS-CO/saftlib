# Function Generator Test

What is a parameter set?
Where does this piece of saft sit?

## `test-fg`
## `test-mfg`
## `test-fg-performance`
## `test-mfg-duration`

```cpp
time_left -= DURATIONS[result.back().step][result.back().freq];
```


For MILbus we need at least
```
uint8_t step = 3 + static_cast<unsigned char>(distribution(generator) * 4);
uint8_t freq = 0 + static_cast<unsigned char>(distribution(generator) * 7);
```

`std::chrono::nanoseconds` defaults to `int64` with maximum  9.223.372.036.854.775.807 ns = 166 days 3 hours 42 minutes 31.14 s
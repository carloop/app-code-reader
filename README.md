# Fault code reader for [Carloop](https://carloop.io)

Reads engine diagnostic trouble codes on CAN at 500 kbit and outputs them formatted to the USB Serial and published as Particle events.

## [Flash it to your Carloop and use it right away](https://carloop.github.io/app-code-reader)

## Building

Copy this application to Particle Build and add the [Carloop library](https://build.particle.io/libs/56eebf35e1b20225ce00048d)

## Example fault codes

```
# Mode 03 codes: none here
{"timestamp":670.646000,"bus":1,"id":"0x7df","data":"0x0103000000000000"}
{"timestamp":670.651000,"bus":1,"id":"0x7e8","data":"0x0243000000000000"}

# Mode 07 codes: three here
{"timestamp":685.359000,"bus":1,"id":"0x7df","data":"0x0107000000000000"}
{"timestamp":685.362000,"bus":1,"id":"0x7e8","data":"0x10 08 47 03 0102 0113"}
{"timestamp":685.362000,"bus":1,"id":"0x7e0","data":"0x3000000000000000"}
{"timestamp":685.364000,"bus":1,"id":"0x7e8","data":"0x21 0010 0000000000"}

# Mode 0a codes: none here
{"timestamp":734.808000,"bus":1,"id":"0x7df","data":"0x010a000000000000"}
{"timestamp":734.812000,"bus":1,"id":"0x7e8","data":"0x024a000000000000"}
```

## Testing

To run the unit tests, run `make` in the root of the repository.

## License

Copyright 2016 1000 Tools, Inc.

Distributed under the MIT license. See [LICENSE](/license) for details.


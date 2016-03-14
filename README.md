Modbus protocol emulator based on QT
====================================
Feature
-------
* Support modbus RTU (master/slave) and modbus TCP (master/slave)
* Support function code 1 ~ 6
* Support excption code 1 ~ 4
* Support respons time display

Tools
------------
* QT5.5
* qmake

How to compile
---------------
1. `qmake` -> to generate Makfile
2. `make`
3. `sudo ./QTmbus`

Bug
---
* different UI style between `sudo` and `normal`
* check argument at first !

cd ../esp/v5.3/esp-idf
run "export.bat" => to init idf.py
cd to this project and run "idf.py flash monitor", have some condition:
turn on "Bluetooth" in "Component config" by "idf.py menuconfig"
turn on "Single factory app (large), no OTA)" in "Partition Table" by "idf.py menuconfig"

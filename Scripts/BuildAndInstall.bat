cd ../
mkdir build
cd build
call cmake ../
cd ../
call cmake --build build --config Release
call cmake --install build --config Release
sudo apt-get install g++-4.8 g++ git make libboost1.54-all-dev libssl-dev cmake
sudo apt-get install libopencv-dev
sudo apt-get install apache2
sudo chmod -R 777 /var/www
git clone https://git.codeplex.com/casablanca
cd casablanca/Release
mkdir build.release
cd build.release
CXX=g++-4.8 cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
sudo ldconfig
echo "All dependencies have been installed"
echo "You can now build R7IntensityPro running 'make'"

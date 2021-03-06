sudo apt-get install g++-4.8 g++ git make libboost1.54-all-dev libssl-dev cmake gcc
sudo apt-get install libopencv-dev
sudo apt-get install apache2 php5 php5-curl
sudo apt-get install dkms
sudo apt-get install libtesseract-dev tesseract-ocr tesseract-ocr-fra
sudo dpkg -i BlackmagicSoftware/deb/amd64/desktopvideo_10.5a17_amd64.deb
sudo dpkg -i BlackmagicSoftware/deb/amd64/desktopvideo-gui_10.5a17_amd64.deb
sudo dpkg -i BlackmagicSoftware/deb/amd64/mediaexpress_3.4.1a2_amd64.deb
sudo chmod -R 777 /var/www
cp html/* /var/www/html
git clone https://git.codeplex.com/casablanca
cd casablanca/Release
mkdir build.release
cd build.release
CXX=g++-4.8 cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
sudo ldconfig
echo "All dependencies have been installed"
echo "You can now build VideoWatchdog running 'make'"

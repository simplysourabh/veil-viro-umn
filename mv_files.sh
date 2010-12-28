# go to the directory where I downloaded the files from the svn
cd ~/veil-viro-umn

# now copy the vid.hh to include/click
cp vid.hh /opt/click/include/click/


# copy confparse.cc to click/lib
cp confparse.cc /opt/click/lib/confparse.cc

# copy confparse.hh to include/click/
cp confparse.hh /opt/click/include/click/confparse.hh 

# copy veil directory to click/elements/
mkdir /opt/click/elements/veil
cp veil/*.hh /opt/click/elements/veil/ 
cp veil/*.cc /opt/click/elements/veil/


# now copy the configure.ac and Makefile.in files too.
cp configure.ac /opt/click/elements/veil/
cp Makefile.in /opt/click/elements/veil/

cd /opt/click/elements/veil
#sudo ./configure
#sudo make elemlist
#sudo make 
#sudo make install 

# now go the click root directory run configure, make, make install
cd /opt/click/
sudo ./configure  --enable-analysis --enable-test --enable-userlevel --enable-local
sudo make elemlist
sudo make 
sudo make install

# go to veil directory and run configure, make and make install
cd elements/veil
sudo ./configure
sudo make elemlist
sudo make 
sudo make install 

cd ~/veil-viro-umn 
#

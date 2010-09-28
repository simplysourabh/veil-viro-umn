# go to the directory where I downloaded the files from the svn
cd ~/veil-viro-umn

# now copy the vid.hh to include/click
#cp vid.hh /opt/ns-allinone-2.29/click/include/click/


# copy confparse.cc to click/lib
#cp confparse.cc /opt/ns-allinone-2.29/click/lib/confparse.cc

# copy confparse.hh to include/click/
#cp confparse.hh /opt/ns-allinone-2.29/click/include/click/confparse.hh 

# copy veil directory to click/elements/
cp veil/*.hh /opt/ns-allinone-2.29/click/elements/veil/ 
cp veil/*.cc /opt/ns-allinone-2.29/click/elements/veil/

# now copy the configure.ac and Makefile.in files too.
#cp configure.ac /opt/ns-allinone-2.29/click/elements/veil/
#cp Makefile.in /opt/ns-allinone-2.29/click/elements/veil/

cd /opt/ns-allinone-2.29/click/elements/veil
#sudo ./configure
#sudo make elemlist
#sudo make 
sudo make install 

# now go the click root directory run configure, make, make install
cd /opt/ns-allinone-2.29/click/
#sudo ./configure  --enable-analysis --enable-test --enable-userlevel --enable-local
#sudo make elemlist
#sudo make 
sudo make install

# go to veil directory and run configure, make and make install
cd elements/veil
sudo ./configure
sudo make elemlist
sudo make 
sudo make install 

cd ~/veil-viro-umn 
#

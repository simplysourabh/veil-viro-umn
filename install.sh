#LICENSE START
# Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
# Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
#LICENSE END
## install git, vim, g++
sudo apt-get install git
sudo apt-get install vim
sudo apt-get install g++

## download click
cd /opt
git clone git://read.cs.ucla.edu/click click

## download veil
cd ~
svn checkout https://veil-viro-umn.googlecode.com/svn/ veil-viro-umn --username simply.sourabh


## copy the veil files.
## go to the directory where I downloaded the files from the svn
cd ~/veil-viro-umn
## now copy the vid.hh to include/click
cp vid.hh /opt/click/include/click/
## copy confparse.cc to click/lib
cp confparse.cc /opt/click/lib/confparse.cc
## copy confparse.hh to include/click/
cp confparse.hh /opt/click/include/click/confparse.hh 
## copy veil directory to click/elements/
mkdir /opt/click/elements/veil
cp veil/*.hh /opt/click/elements/veil/ 
cp veil/*.cc /opt/click/elements/veil/
## now copy the configure.ac and Makefile.in files too.
cp configure.ac /opt/click/elements/veil/
cp Makefile.in /opt/click/elements/veil/

cd /opt/click/
## IMPORTANT: edit the ./configure and add the option "--enable-veil" below the "--enable-local" option
sudo ./configure  --enable-analysis --enable-test --enable-userlevel --enable-local --enable-veil
sudo make elemlist
sudo make 
sudo make install

cd ~/veil-viro-umn 

## now run the python script to generate the configuration click file
python generateClickConf.py <fill vccmac HERE::IMPORANT::> <OPTIONAL:: write the output click file name>

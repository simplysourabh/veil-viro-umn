#!/usr/bin/python
#LICENSE START
# Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
# Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
#LICENSE END
# This program simply adds a license as a header to all the 
# files containing the source code.

# the input is the directory path (all the files under
# this directory and sub-directories are considered) 
# 2. the file types

import sys,os,shutil

def updateSourceFile(filename,licensestr,commentstr):
	fout = open(filename+'.tmp', 'w')
	fin = open(filename,'r')
	ignorefirstline = False
	for line in fin:
		if line.startswith('#!/'):
			fout.write(line)
			ignorefirstline = True

	fout.write(commentstr+'LICENSE START\n')
	for line in licensestr:
		fout.write(commentstr+' '+line)
	fout.write(commentstr+'LICENSE END\n')
	fin.close()
	fin = open(filename,'r')
	if ignorefirstline:
		fin.readline()

	ignore = False
	for line in fin:
		if line.startswith(commentstr+'LICENSE START'):
			print 'Old license found.'
			ignore = True
			continue
		if line.startswith(commentstr+'LICENSE END'):
			ignore = False
			continue
		if ignore:
			print 'Deleting line:',line.strip()
			continue
		fout.write(line)
	
	fout.close()
	fin.close()
	shutil.copymode(filename, filename+'.tmp')
	os.remove(filename)
	os.rename(filename+'.tmp', filename)

if len(sys.argv) < 4:
	print 'Usage:',sys.argv[0],' <direcotry> <license_file> <extension> '
	print 'For example: "', sys.argv[0], 'veil .cc .hh" will add the license to all the files found under directory veil with extension .cc and .hh'
	sys.exit(0)

dirname = sys.argv[1]
license = []
f = open(sys.argv[2])
for line in f:
	license.append(line)

extensions = sys.argv[3:]
comment = {}
comment['.cc'] = '//'
comment['.hh'] = '//'
comment['.c'] = '//'
comment['.h'] = '//'
comment['.cpp'] = '//'
comment['.click'] = '//'
comment['.java'] = '//'
comment['.py'] = '#'
comment['.sh'] = '#'
comment['.txt'] = '//'

for e in extensions:
	if e not in comment:
		print 'Extension:',e,'is unknown for me! Please add the comment character for it in the list. For now I will quit.'
		sys.exit()


for root,dirs,files in os.walk(dirname):
	for name in files:
		matchFound = False
		for e in extensions:
			if name.endswith(e):
				updateSourceFile(os.path.join(root,name), license, comment[e])
				print ('++ Updated file: '+os.path.join(root,name))
				matchFound = True
				break
		if not matchFound:
			print ('-- Ignored file: '+ os.path.join(root,name))


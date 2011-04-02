#!/usr/bin/python

import filecmp
import os
import sys
import shlex
import subprocess
import json

def msg(msg):
	print msg+"...",

def result(msg):
	print msg;

def failure(msg):
	type, value = sys.exc_info()[:2]
	if None == msg:
		msg=value
	print msg;
	sys.exit(msg)


if "nt"==os.name:
	TMP_DIR="win32"+os.sep+"Debug"
else:
	try:
		msg("Reading configuration...")
		f=open("build"+os.sep+"configure.ctx", "r")
		ctx=eval(f.read())
		f.close()
		result("OK")
	except:
		failure("Can not open \"configure.ctx\". Try running ./configure first!")
	msg("Guessing platform...")
	if 1==len(ctx["platforms"]):
		platform=ctx["platforms"][0]
		result(platform)
		TMP_DIR="build"+os.sep+platform
	else:
		failure("Error. Please configure with exactly one platform!")
BUILD_DIR=TMP_DIR
DOCS_DIR="test_docs"


def tmp(filename):
	return os.path.join(TMP_DIR, filename)

def build(filename):
	return os.path.join(BUILD_DIR, filename)

def docs(filename):
	return os.path.join(DOCS_DIR, filename)


def rm(filename):
	try:
		msg("remove "+filename)
		if os.path.exists(filename):
			os.remove(filename)
		result("OK")
	except:
		failure(None)
		
def call(path, pre, args, outfile, post):
	try:
		msg("executing "+path)
		_args = [ path ]
		_args.extend(args)
		err = open(tmp("stderr.txt"), "w")
		out = open(outfile, "w")
		ret=subprocess.call(_args, stdout=out, stderr=err)
		out.close()
		err.close()
		result("OK")
	except:
		failure(None)
	
def normlinebreak(path):
	try:
		msg("normalize line breaks "+path+"...")		
		newlines = []
		changed  = False
		for line in open(path, 'rb').readlines():
			if line[-2:] == '\r\n':
				line = line[:-2] + '\n'
				changed = True
			newlines.append(line)
		if changed:
			open(path, 'wb').writelines(newlines)
			result("Changed")
		else:
			result("OK")
	except:
		failure(None)
	

def regr(regrpath, path, normalize):
	if normalize:
		normlinebreak(regrpath)
		normlinebreak(path)	
	msg("regression testing "+path+" against "+regrpath)
	eq=filecmp.cmp(regrpath, path)
	if (eq):
		result("OK")
	else:
		failure("Regression test failed!")

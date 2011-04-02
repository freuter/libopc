#!/usr/bin/python

import filecmp
import os
import sys
import shlex
import subprocess

TMP_DIR="win32"+os.sep+"Debug"
BUILD_DIR="win32"+os.sep+"Debug"
DOCS_DIR="test_docs"

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
		err = open(tmp("stderr,txt"), "w")
		out = open(outfile, "w")
		ret=subprocess.call(_args, stdout=out, stderr=err)
		out.close()
		err.close()
		result("OK")
	except:
		failure(None)
	

def regr(regrpath, path):
	msg("regression testing "+path+" against "+regrpath)
	eq=filecmp.cmp(regrpath, path)
	if (eq):
		result("OK")
	else:
		failure("Regression test failed!")

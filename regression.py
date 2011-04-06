#!/usr/bin/python

import filecmp
import os
import sys
import shlex
import subprocess
import json
import shutil

def msg(msg):
	print msg+"...",

def result(msg):
	print msg;

def failure(msg):
	type, value = sys.exc_info()[:2]
	if None == msg:
		msg=value
#	raise
#	print msg
	sys.exit(msg)



def init(mode):
	global TARGET_MODE
	global TMP_DIR
	global BUILD_DIR
	global DOCS_DIR
	TARGET_MODE=mode.lower()
	if "nt"==os.name:
		TMP_DIR="win32"+os.sep+mode
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

def cp(src, dst):
	try:
		msg("copy "+src+" to "+dst)
		shutil.copyfile(src, dst)
		result("OK")
	except:
		failure(None)

def ensureDir(dir):
	try:
		if not(os.path.exists(dir)):
			msg("creating dir "+dir)
			os.mkdir(dir)
			result("OK")
	except:
		failure(None)


def call(path, pre, args, outfile, post):
	try:
		msg("executing "+path)
		if os.path.exists(tmp("stderr.txt")):
			os.remove(tmp("stderr.txt"))
		if os.path.exists(outfile):
			os.remove(outfile)
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

def compile(path):
	try:
		msg("compiling "+path)
		if os.path.exists(tmp("stderr.txt")):
			os.remove(tmp("stderr.txt"))
		if os.path.exists(tmp("stdout.txt")):
			os.remove(tmp("stdout.txt"))
		err = open(tmp("stderr.txt"), "w")
		out = open(tmp("stdout.txt"), "w")
		exe=os.path.abspath(os.path.splitext(path)[0]+".exe")
		obj=os.path.abspath(os.path.splitext(path)[0]+".obj")
		if TARGET_MODE == "debug":
			rtl="/MDd"
		else:
			rtl="/MD"
		cl_args=["cl", 
			os.path.abspath(path), 
			"/Fo"+obj,
			"/TP", "/c", rtl,
			"/I..\\..\\config\\win32-debug-msvc\\zlib-1.2.5",
			"/I..\\..\\third_party\\zlib-1.2.5",
			"/I..\\..\\config\\win32-debug-msvc\\libxml2-2.7.7",
			"/I..\\..\\third_party\\libxml2-2.7.7\\include\\libxml",
			"/I..\\..\\third_party\\libxml2-2.7.7\\include",
			"/I..\\..\\config\\win32-debug-msvc\\plib\\include",
			"/I..\\..\\config",
			"/I..\\..",
			"/D", "WIN32",
			"/D", "LIBXML_STATIC",
			"/D","_UNICODE",
			"/D", "UNICODE" ]
		link_args=["link", "/OUT:"+os.path.abspath(exe),  os.path.abspath(os.path.splitext(path)[0]+".obj"),"/NOLOGO", "/MACHINE:X86",
			"kernel32.lib", "user32.lib", "gdi32.lib", "winspool.lib", "comdlg32.lib", "advapi32.lib", "shell32.lib", "ole32.lib", "oleaut32.lib", "uuid.lib", "odbc32.lib", "odbccp32.lib", 
			"opc.lib", "plib.lib", "zlib.lib", "xml.lib" ]
		cwd=os.getcwd()
		os.chdir(tmp(""))
		ret=subprocess.call(cl_args, stdout=out, stderr=err)
		if ret==0:
			ret=subprocess.call(link_args, stdout=out, stderr=err)
		os.chdir(cwd)
		out.close()
		err.close()
		if ret==0:
			result("OK")
		else:
			out = open(tmp("stdout.txt"), "r")
			out_str=out.read()
			out.close()
			failure("FAIL:\n"+out_str)
		return exe
	except:
		failure(None)